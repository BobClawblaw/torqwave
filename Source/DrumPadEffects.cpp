#include "DrumPadEffects.h"

namespace DrumMachine
{
namespace
{
    constexpr double pi = juce::MathConstants<double>::pi;

    // In the order of Fx.
    constexpr FxInfo infos[numFx]
    {
        { "envOn",        "Envelope",          0.0f,    1.0f,     0.0f,    "",    1.0f },
        { "envAttack",    "Attack",            0.0f,    500.0f,   0.0f,    "ms",  0.35f },
        { "envHold",      "Hold",              0.0f,    2000.0f,  50.0f,   "ms",  0.35f },
        { "envDecay",     "Decay",             5.0f,    5000.0f,  300.0f,  "ms",  0.3f },
        { "envCurve",     "Curve",             -100.0f, 100.0f,   0.0f,    "",    1.0f },
        { "transOn",      "Transient",         0.0f,    1.0f,     0.0f,    "",    1.0f },
        { "transAttack",  "Attack",            -100.0f, 100.0f,   0.0f,    "%",   1.0f },
        { "transSustain", "Sustain",           -100.0f, 100.0f,   0.0f,    "%",   1.0f },
        { "satOn",        "Saturate",          0.0f,    1.0f,     0.0f,    "",    1.0f },
        { "satType",      "Curve",             0.0f,    3.0f,     0.0f,    "",    1.0f },
        { "satAmount",    "Amount",            0.0f,    100.0f,   40.0f,   "%",   1.0f },
        { "satMix",       "Mix",               0.0f,    100.0f,   100.0f,  "%",   1.0f },
        { "eqOn",         "EQ",                0.0f,    1.0f,     0.0f,    "",    1.0f },
        { "eqLowFreq",    "Low Shelf",         20.0f,   2000.0f,  100.0f,  "Hz",  0.35f },
        { "eqLowGain",    "Low Shelf Gain",    -18.0f,  18.0f,    0.0f,    "dB",  1.0f },
        { "eqBell1Freq",  "Bell 1",            20.0f,   20000.0f, 400.0f,  "Hz",  0.25f },
        { "eqBell1Gain",  "Bell 1 Gain",       -18.0f,  18.0f,    0.0f,    "dB",  1.0f },
        { "eqBell1Width", "Bell 1 Width",      0.1f,    4.0f,     1.0f,    "oct", 0.5f },
        { "eqBell2Freq",  "Bell 2",            20.0f,   20000.0f, 2500.0f, "Hz",  0.25f },
        { "eqBell2Gain",  "Bell 2 Gain",       -18.0f,  18.0f,    0.0f,    "dB",  1.0f },
        { "eqBell2Width", "Bell 2 Width",      0.1f,    4.0f,     1.0f,    "oct", 0.5f },
        { "eqHighFreq",   "High Shelf",        500.0f,  20000.0f, 8000.0f, "Hz",  0.35f },
        { "eqHighGain",   "High Shelf Gain",   -18.0f,  18.0f,    0.0f,    "dB",  1.0f },
        { "compOn",       "Compressor",        0.0f,    1.0f,     0.0f,    "",    1.0f },
        { "compThreshold","Threshold",         -60.0f,  0.0f,     -18.0f,  "dB",  1.0f },
        { "compRatio",    "Ratio",             1.0f,    20.0f,    4.0f,    ":1",  0.5f },
        { "compAttack",   "Attack",            0.1f,    100.0f,   5.0f,    "ms",  0.35f },
        { "compRelease",  "Release",           10.0f,   2000.0f,  120.0f,  "ms",  0.35f },
        { "compGain",     "Gain",              0.0f,    24.0f,    0.0f,    "dB",  1.0f },
    };

    bool isSwitch (int fx)   { return fx == envOn || fx == transOn || fx == satOn || fx == eqOn || fx == compOn; }

    /** The power the envelope's fall is raised to: 1 a straight line; more, it falls fast and
        lingers; less, it stays up and drops at the end. */
    double envelopePower (float curve)   { return std::exp2 (2.5 * juce::jlimit (-1.0, 1.0, (double) curve * 0.01)); }

    //==============================================================================
    // The saturator's curves, each given the signal already driven.
    constexpr float tubeBias = 0.35f;

    float tape (float x)   { return x / std::sqrt (1.0f + x * x); }
    float tube (float x)   { return std::tanh (x + tubeBias) - std::tanh (tubeBias); }

    float shaped (int curve, float x)
    {
        switch (curve)
        {
            case 0:   return tape (x);
            case 1:   return tube (x);
            case 2:   return juce::jlimit (-1.0f, 1.0f, x);
            default:  return std::sin (juce::MathConstants<float>::halfPi * x);
        }
    }

    /** How hard a curve is driven at an amount (0 to 1), and what brings its output back to full
        scale for a full-scale input. Tape and Tube start nearly straight; Clip and Fold start at
        the signal as it is. */
    void saturationSet (int curve, float amount, float& drive, float& norm)
    {
        if (curve <= 1)
        {
            drive = 0.25f * std::exp2 (6.0f * amount);
            norm = curve == 0 ? 1.0f / tape (drive) : 1.0f / juce::jmax (std::abs (tube (drive)), std::abs (tube (-drive)));
        }
        else
        {
            drive = std::exp2 (4.0f * amount);
            norm = 1.0f;
        }
    }

    //==============================================================================
    struct Coefficients { double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0; };

    double limitedHz (double hz, double rate)   { return juce::jlimit (10.0, rate * 0.45, hz); }

    /** The Audio EQ Cookbook's peaking band, its width in octaves between its half-gain points. */
    Coefficients bell (double hz, double gainDb, double octaves, double rate)
    {
        const auto A = std::pow (10.0, gainDb / 40.0);
        const auto w = 2.0 * pi * limitedHz (hz, rate) / rate;
        const auto alpha = std::sin (w) * std::sinh (std::log (2.0) * 0.5 * juce::jlimit (0.1, 4.0, octaves) * w / std::sin (w));
        const auto a0 = 1.0 + alpha / A;
        return { (1.0 + alpha * A) / a0, -2.0 * std::cos (w) / a0, (1.0 - alpha * A) / a0, -2.0 * std::cos (w) / a0, (1.0 - alpha / A) / a0 };
    }

    /** Its shelves, at the slope that neither rings nor sags (S = 1). */
    Coefficients shelf (bool high, double hz, double gainDb, double rate)
    {
        const auto A = std::pow (10.0, gainDb / 40.0);
        const auto w = 2.0 * pi * limitedHz (hz, rate) / rate;
        const auto c = std::cos (w), alpha = std::sin (w) * 0.5 * std::sqrt (2.0), k = 2.0 * std::sqrt (A) * alpha;
        const auto s = high ? -1.0 : 1.0;   // the low shelf's signs; the high shelf's are the other way
        const auto a0 = (A + 1.0) + s * (A - 1.0) * c + k;
        return { A * ((A + 1.0) - s * (A - 1.0) * c + k) / a0,
                 s * 2.0 * A * ((A - 1.0) - s * (A + 1.0) * c) / a0,
                 A * ((A + 1.0) - s * (A - 1.0) * c - k) / a0,
                 -s * 2.0 * ((A - 1.0) + s * (A + 1.0) * c) / a0,
                 ((A + 1.0) + s * (A - 1.0) * c - k) / a0 };
    }

    /** The bands of an EQ that are not at 0 dB, in order. */
    int eqBands (const PadEffects& fx, double rate, std::array<Coefficients, 4>& into)
    {
        int n = 0;

        if (! juce::exactlyEqual (fx[eqLowGain], 0.0f))
            into[(size_t) n++] = shelf (false, fx[eqLowFreq], fx[eqLowGain], rate);

        if (! juce::exactlyEqual (fx[eqBell1Gain], 0.0f))
            into[(size_t) n++] = bell (fx[eqBell1Freq], fx[eqBell1Gain], fx[eqBell1Width], rate);

        if (! juce::exactlyEqual (fx[eqBell2Gain], 0.0f))
            into[(size_t) n++] = bell (fx[eqBell2Freq], fx[eqBell2Gain], fx[eqBell2Width], rate);

        if (! juce::exactlyEqual (fx[eqHighGain], 0.0f))
            into[(size_t) n++] = shelf (true, fx[eqHighFreq], fx[eqHighGain], rate);

        return n;
    }

    //==============================================================================
    constexpr float compressorKnee = 6.0f;

    /** How far a compressor turns down a level that is `over` its threshold, in dB (0 or less). */
    float compressorReduction (float over, float slope)
    {
        if (over > compressorKnee * 0.5f)
            return over * slope;

        if (over > -compressorKnee * 0.5f)
            return slope * (over + compressorKnee * 0.5f) * (over + compressorKnee * 0.5f) / (2.0f * compressorKnee);

        return 0.0f;
    }
}

//==============================================================================
const FxInfo& fxInfo (int fx)
{
    return infos[juce::jlimit (0, (int) numFx - 1, fx)];
}

juce::StringArray saturationNames()   { return { "Tape", "Tube", "Clip", "Fold" }; }
juce::StringArray effectNames()       { return { "Envelope", "Transient", "Saturate", "EQ", "Compressor" }; }

PadEffects::PadEffects()
{
    for (size_t i = 0; i < values.size(); ++i)
        values[i] = infos[i].start;
}

bool PadEffects::anyOn() const noexcept
{
    return std::any_of (effectSwitches.begin(), effectSwitches.end(), [this] (Fx f) { return on (f); });
}

void PadEffects::limit()
{
    for (int i = 0; i < numFx; ++i)
    {
        auto& v = values[(size_t) i];
        v = juce::jlimit (infos[i].low, infos[i].high, v);

        if (isSwitch (i))
            v = v >= 0.5f ? 1.0f : 0.0f;
        else if (i == satType)
            v = std::round (v);
    }
}

bool PadEffects::operator== (const PadEffects& o) const noexcept
{
    for (size_t i = 0; i < values.size(); ++i)
        if (! juce::exactlyEqual (values[i], o.values[i]))
            return false;

    return true;
}

//==============================================================================
double envelopeSeconds (const PadEffects& fx)
{
    return ((double) fx[envAttack] + (double) fx[envHold] + (double) fx[envDecay]) * 0.001;
}

float envelopeGain (const PadEffects& fx, double seconds)
{
    const auto a = fx[envAttack] * 0.001, h = fx[envHold] * 0.001, d = juce::jmax (0.001, fx[envDecay] * 0.001);

    if (seconds < 0.0)
        return 0.0f;

    if (seconds < a)
        return (float) (seconds / a);

    if (seconds < a + h)
        return 1.0f;

    const auto x = (seconds - a - h) / d;
    return x >= 1.0 ? 0.0f : (float) std::pow (1.0 - x, envelopePower (fx[envCurve]));
}

float saturated (const PadEffects& fx, float in)
{
    const auto curve = juce::jlimit (0, 3, juce::roundToInt (fx[satType]));
    float drive, norm;
    saturationSet (curve, juce::jlimit (0.0f, 1.0f, fx[satAmount] * 0.01f), drive, norm);
    const auto wet = shaped (curve, drive * in) * norm;
    const auto mix = juce::jlimit (0.0f, 1.0f, fx[satMix] * 0.01f);
    return mix >= 1.0f ? wet : in + mix * (wet - in);
}

float eqGainDb (const PadEffects& fx, double hz, double sampleRate)
{
    std::array<Coefficients, 4> bands;
    const auto n = eqBands (fx, sampleRate, bands);
    const auto w = 2.0 * pi * juce::jlimit (1.0, sampleRate * 0.499, hz) / sampleRate;
    const std::complex<double> z1 = std::polar (1.0, -w), z2 = std::polar (1.0, -2.0 * w);
    auto db = 0.0;

    for (int i = 0; i < n; ++i)
    {
        const auto& c = bands[(size_t) i];
        db += 20.0 * std::log10 (juce::jmax (1.0e-9, std::abs ((c.b0 + c.b1 * z1 + c.b2 * z2) / (1.0 + c.a1 * z1 + c.a2 * z2))));
    }

    return (float) db;
}

float compressedDb (const PadEffects& fx, float inDb)
{
    const auto slope = 1.0f / juce::jmax (1.0f, fx[compRatio]) - 1.0f;
    return inDb + compressorReduction (inDb - fx[compThreshold], slope) + fx[compGain];
}

//==============================================================================
bool HitEffects::start (const PadEffects& fx, double rate)
{
    *this = {};

    if (! fx.anyOn())
        return false;

    const auto coefficient = [rate] (double seconds) { return (float) (1.0 - std::exp (-1.0 / (juce::jmax (1.0e-5, seconds) * rate))); };

    if (fx.on (envOn))
    {
        env = true;
        envA = fx[envAttack] * 0.001 * rate;
        envH = fx[envHold] * 0.001 * rate;
        envD = juce::jmax (1.0, fx[envDecay] * 0.001 * rate);
        envPower = envelopePower (fx[envCurve]);
        envelopeLength = juce::jmax ((juce::int64) 2, (juce::int64) std::ceil (envA + envH + envD));
    }

    if (fx.on (transOn) && ! (juce::exactlyEqual (fx[transAttack], 0.0f) && juce::exactlyEqual (fx[transSustain], 0.0f)))
    {
        // At the most 12 dB either way from each, reached when the followers are three octaves apart.
        trans = true;
        tFastK = coefficient (0.0005);
        tSlowK = coefficient (0.020);
        tFallK = coefficient (0.040);
        tHeldK = coefficient (0.300);
        tAttack = juce::jlimit (-1.0f, 1.0f, fx[transAttack] * 0.01f) * (2.0f / 3.0f);
        tSustain = juce::jlimit (-1.0f, 1.0f, fx[transSustain] * 0.01f) * (2.0f / 3.0f);
    }

    if (fx.on (satOn) && fx[satMix] > 0.0f)
    {
        sat = true;
        satCurve = juce::jlimit (0, 3, juce::roundToInt (fx[satType]));
        saturationSet (satCurve, juce::jlimit (0.0f, 1.0f, fx[satAmount] * 0.01f), satDrive, satNorm);
        satWet = juce::jlimit (0.0f, 1.0f, fx[satMix] * 0.01f);
        dcK = coefficient (1.0 / (2.0 * pi * 12.0));   // Tube's curve is lopsided: what it adds below 12 Hz is taken off
    }

    if (fx.on (eqOn))
    {
        std::array<Coefficients, 4> set;
        numBands = eqBands (fx, rate, set);
        eq = numBands > 0;

        for (int i = 0; i < numBands; ++i)
        {
            const auto& c = set[(size_t) i];
            auto& b = bands[(size_t) i];
            b.b0 = c.b0;  b.b1 = c.b1;  b.b2 = c.b2;  b.a1 = c.a1;  b.a2 = c.a2;
        }
    }

    if (fx.on (compOn))
    {
        comp = true;
        cAttack = coefficient (fx[compAttack] * 0.001);
        cRelease = coefficient (fx[compRelease] * 0.001);
        cThreshold = fx[compThreshold];
        cSlope = 1.0f / juce::jmax (1.0f, fx[compRatio]) - 1.0f;
        cMakeUp = juce::Decibels::decibelsToGain (fx[compGain]);
    }

    return env || trans || sat || eq || comp;
}

void HitEffects::run (float& l, float& r, juce::int64 age) noexcept
{
    if (env)
    {
        const auto t = (double) age;
        auto g = 1.0f;

        if (t < envA)
            g = (float) (t / envA);
        else if (t >= envA + envH)
        {
            const auto x = (t - envA - envH) / envD;
            g = x >= 1.0 ? 0.0f : juce::exactlyEqual (envPower, 1.0) ? (float) (1.0 - x) : (float) std::pow (1.0 - x, envPower);
        }

        l *= g;
        r *= g;
    }

    if (trans)
    {
        // A fast follower of the level; a slow one that lags it on the way up, so the two are
        // apart only while a hit begins; and one that holds on after it, so those two are apart
        // only while it dies away. Each difference, in octaves, moves the gain.
        constexpr auto floor = 1.0e-4f;
        const auto level = juce::jmax (std::abs (l), std::abs (r));
        tFast += (level > tFast ? tFastK : tFallK) * (level - tFast);
        tSlow = tFast > tSlow ? tSlow + tSlowK * (tFast - tSlow) : tFast;
        tHeld = juce::jmax (tFast, tHeld + tHeldK * (tFast - tHeld));
        const auto rising = juce::jmin (3.0f, std::log2 ((tFast + floor) / (tSlow + floor)));
        const auto falling = juce::jmin (3.0f, std::log2 ((tHeld + floor) / (tFast + floor)));
        const auto g = std::exp2 (tAttack * rising + tSustain * falling);
        l *= g;
        r *= g;
    }

    if (sat)
    {
        auto wl = shaped (satCurve, satDrive * l) * satNorm;
        auto wr = shaped (satCurve, satDrive * r) * satNorm;

        if (satCurve == 1)
        {
            dcL += dcK * (wl - dcL);
            dcR += dcK * (wr - dcR);
            wl -= dcL;
            wr -= dcR;
        }

        // All of it at a mix of 100 %, exactly.
        l = satWet >= 1.0f ? wl : l + satWet * (wl - l);
        r = satWet >= 1.0f ? wr : r + satWet * (wr - r);
    }

    if (eq)
    {
        auto xl = (double) l, xr = (double) r;

        for (int i = 0; i < numBands; ++i)
        {
            auto& b = bands[(size_t) i];
            const auto yl = b.b0 * xl + b.l1;
            b.l1 = b.b1 * xl - b.a1 * yl + b.l2;
            b.l2 = b.b2 * xl - b.a2 * yl;
            const auto yr = b.b0 * xr + b.r1;
            b.r1 = b.b1 * xr - b.a1 * yr + b.r2;
            b.r2 = b.b2 * xr - b.a2 * yr;
            xl = yl;
            xr = yr;
        }

        l = (float) xl;
        r = (float) xr;
    }

    if (comp)
    {
        // Both sides turned down together, by the louder. Its level is its peaks', let go over the
        // release; how far that says to turn it down is come to over the attack.
        const auto level = juce::jmax (std::abs (l), std::abs (r));
        cPeak = level > cPeak ? level : cPeak + cRelease * (level - cPeak);
        const auto wanted = compressorReduction (juce::Decibels::gainToDecibels (cPeak, -120.0f) - cThreshold, cSlope);
        cReduction = wanted < cReduction ? cReduction + cAttack * (wanted - cReduction) : wanted;
        reductionDb = juce::jmin (reductionDb, cReduction);
        const auto g = juce::Decibels::decibelsToGain (cReduction) * cMakeUp;
        l *= g;
        r *= g;
    }
}
}
