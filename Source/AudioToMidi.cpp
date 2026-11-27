#include "AudioToMidi.h"
#include "PitchCorrection.h"
#include <cmath>

namespace AudioToMidi
{
namespace
{
    float rmsOver (const AudioSnapshot& audio, juce::int64 from, juce::int64 to) noexcept
    {
        const auto chs = audio.getNumChannels();
        const auto len = audio.getNumFrames();
        const auto a = juce::jlimit ((juce::int64) 0, len, from);
        const auto b = juce::jlimit (a, len, to);
        if (chs <= 0 || b <= a)
            return 0.0f;

        const auto n = (int) std::min<juce::int64> (b - a, 480000);
        juce::AudioBuffer<float> tmp (chs, n);
        audio.read (tmp, 0, a, n);

        double sumSq = 0.0;
        for (int ch = 0; ch < chs; ++ch)
        {
            const auto* p = tmp.getReadPointer (ch);
            for (int i = 0; i < n; ++i)
                sumSq += (double) p[i] * (double) p[i];
        }
        return (float) std::sqrt (sumSq / (double) ((juce::int64) n * chs));
    }

    float levelToVelocity (float rms) noexcept
    {
        const auto db = juce::Decibels::gainToDecibels (rms, -80.0f);
        return juce::jlimit (0.18f, 1.0f, (db + 48.0f) / 42.0f);
    }

    juce::int64 quantizeFrame (juce::int64 frame, double sampleRate, double beatSeconds, int division) noexcept
    {
        if (beatSeconds <= 0.0 || division <= 0)
            return frame;
        const auto step = (beatSeconds / (double) division) * sampleRate;
        if (step < 1.0)
            return frame;
        return juce::jmax ((juce::int64) 0, (juce::int64) std::llround (std::round ((double) frame / step) * step));
    }
}

Result extractMelody (const AudioSnapshot& audio, juce::int64 start, juce::int64 end, double sampleRate,
                      const MelodyOptions& options, const std::function<bool (float)>& progress)
{
    Result out;
    if (sampleRate <= 0.0 || end <= start || audio.getNumChannels() <= 0)
        return out;

    const auto track = PitchCorrection::detect (audio, start, end, sampleRate, progress);
    if (track.hz.empty() || track.hop <= 0)
        return out;

    const auto detected = PitchCorrection::findNotes (track);
    const auto minFrames = (juce::int64) std::llround (options.minNoteMs * 0.001 * sampleRate);
    const auto minRms = juce::Decibels::decibelsToGain (options.minLevelDb);

    for (const auto& n : detected)
    {
        const auto absStart = track.first + (juce::int64) n.first * track.hop;
        const auto absEnd = track.first + (juce::int64) n.end * track.hop;
        const auto len = absEnd - absStart;
        if (len < minFrames || n.pitch <= 0.0)
            continue;

        const auto rms = rmsOver (audio, absStart, absEnd);
        if (rms < minRms)
            continue;

        auto pitch = juce::jlimit (0, 127, (int) std::llround (n.pitch));
        if (options.snapScale.has_value())
            pitch = options.snapScale->snap (pitch);

        auto relStart = juce::jmax ((juce::int64) 0, absStart - start);
        if (options.quantizeBeatSeconds > 0.0)
            relStart = quantizeFrame (relStart, sampleRate, options.quantizeBeatSeconds, options.quantizeDivision);

        ClipNote cn;
        cn.pitch = pitch;
        cn.start = relStart;
        cn.length = juce::jmax ((juce::int64) 1, len);
        cn.velocity = levelToVelocity (rms);
        out.notes.push_back (cn);
    }

    // Prevent same-pitch overlaps after quantization
    std::sort (out.notes.begin(), out.notes.end(), [] (const ClipNote& a, const ClipNote& b)
    {
        return a.start < b.start || (a.start == b.start && a.pitch < b.pitch);
    });

    for (size_t i = 0; i + 1 < out.notes.size(); ++i)
        if (out.notes[i].pitch == out.notes[i + 1].pitch && out.notes[i].start + out.notes[i].length > out.notes[i + 1].start)
            out.notes[i].length = juce::jmax ((juce::int64) 1, out.notes[i + 1].start - out.notes[i].start);

    out.summary = juce::String (out.notes.size()) + " melody " + (out.notes.size() == 1 ? "note" : "notes") + " extracted";
    return out;
}

Result extractDrums (const AudioSnapshot& audio, juce::int64 start, juce::int64 end, double sampleRate,
                     const DrumOptions& options, double bpm, const std::function<bool (float)>& progress)
{
    Result out;
    const auto totalLen = end - start;
    const auto numCh = audio.getNumChannels();
    if (sampleRate <= 0.0 || totalLen < 128 || numCh <= 0)
        return out;

    // Mix selection to mono
    juce::AudioBuffer<float> chunk (numCh, (int) totalLen);
    audio.read (chunk, 0, start, (int) totalLen);
    std::vector<float> mono ((size_t) totalLen, 0.0f);
    const auto invCh = 1.0f / (float) numCh;
    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto* src = chunk.getReadPointer (ch);
        for (juce::int64 i = 0; i < totalLen; ++i)
            mono[(size_t) i] += src[i] * invCh;
    }

    // Envelope & spectral-novelty onset detection at 5 ms hop
    const int hop = juce::jmax (32, (int) std::lround (0.005 * sampleRate));
    const int win = hop * 2;
    const int numFrames = juce::jmax (0, (int) ((totalLen - win) / hop));
    if (numFrames < 3)
        return out;

    // Split signal into 3 bands with simple 1-pole crossovers (220 Hz and 3500 Hz) for onset & drum classification
    const auto cLow = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 220.0 / sampleRate));
    const auto cMid = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 3500.0 / sampleRate));

    std::vector<float> eLow ((size_t) numFrames), eMid ((size_t) numFrames), eHigh ((size_t) numFrames), energy ((size_t) numFrames);
    float lp1 = 0.0f, lp2 = 0.0f;

    // Pre-filter into 3 band streams
    std::vector<float> bLow ((size_t) totalLen), bMid ((size_t) totalLen), bHigh ((size_t) totalLen);
    for (juce::int64 i = 0; i < totalLen; ++i)
    {
        const auto x = mono[(size_t) i];
        lp1 += cLow * (x - lp1);
        lp2 += cMid * (x - lp2);
        bLow[(size_t) i] = lp1;
        bMid[(size_t) i] = lp2 - lp1;
        bHigh[(size_t) i] = x - lp2;
    }

    float maxEnergy = 0.0f;
    for (int f = 0; f < numFrames; ++f)
    {
        if (progress != nullptr && (f % 64 == 0) && ! progress (0.5f * (float) f / (float) numFrames))
            return {};

        const auto base = (juce::int64) f * hop;
        double sL = 0.0, sM = 0.0, sH = 0.0;
        for (int k = 0; k < win; ++k)
        {
            const auto idx = (size_t) (base + k);
            sL += (double) bLow[idx] * bLow[idx];
            sM += (double) bMid[idx] * bMid[idx];
            sH += (double) bHigh[idx] * bHigh[idx];
        }
        eLow[(size_t) f] = (float) std::sqrt (sL / win);
        eMid[(size_t) f] = (float) std::sqrt (sM / win);
        eHigh[(size_t) f] = (float) std::sqrt (sH / win);
        energy[(size_t) f] = (float) std::sqrt ((sL + sM + sH) / win);
        maxEnergy = juce::jmax (maxEnergy, energy[(size_t) f]);
    }

    if (maxEnergy < 1.0e-5f)
        return out;

    // Positive half-wave rectified 3-band novelty curve
    std::vector<float> novelty ((size_t) numFrames, 0.0f);
    float maxNov = 0.0f;
    for (int f = 1; f < numFrames; ++f)
    {
        const auto dL = juce::jmax (0.0f, eLow[(size_t) f] - eLow[(size_t) (f - 1)]);
        const auto dM = juce::jmax (0.0f, eMid[(size_t) f] - eMid[(size_t) (f - 1)]);
        const auto dH = juce::jmax (0.0f, eHigh[(size_t) f] - eHigh[(size_t) (f - 1)]);
        novelty[(size_t) f] = dL * 1.1f + dM * 1.0f + dH * 1.25f;
        maxNov = juce::jmax (maxNov, novelty[(size_t) f]);
    }

    // Also inspect frame 0 in case a drum hits right at sample 0
    novelty[0] = eLow[0] * 1.1f + eMid[0] * 1.0f + eHigh[0] * 1.25f;
    maxNov = juce::jmax (maxNov, novelty[0]);

    const auto sens = juce::jlimit (0.0f, 100.0f, options.sensitivity) * 0.01f;
    const auto globalFloor = maxNov * (0.28f - 0.24f * sens);
    const auto minGapFrames = juce::jmax (1, (int) std::lround (options.minGapMs * 0.001 * sampleRate / hop));
    const auto hitNoteLen = juce::jmax ((juce::int64) 1, (juce::int64) std::lround (0.08 * sampleRate));

    int lastHitFrame = -minGapFrames * 2;
    int kicks = 0, snares = 0, hats = 0;

    for (int f = 0; f < numFrames; ++f)
    {
        const auto cur = novelty[(size_t) f];
        const auto prev = f > 0 ? novelty[(size_t) (f - 1)] : 0.0f;
        const auto next = f + 1 < numFrames ? novelty[(size_t) (f + 1)] : 0.0f;
        if (cur < prev || cur < next || cur < globalFloor || f - lastHitFrame < minGapFrames)
            continue;

        // Local mean threshold over +/- 10 frames
        float localSum = 0.0f;
        int localCount = 0;
        for (int k = juce::jmax (0, f - 10); k <= juce::jmin (numFrames - 1, f + 10); ++k)
        {
            localSum += novelty[(size_t) k];
            ++localCount;
        }
        const auto localThresh = (localSum / (float) juce::jmax (1, localCount)) * (2.2f - 1.2f * sens);
        if (cur < localThresh)
            continue;

        lastHitFrame = f;

        // Refine onset sample to steepest energy rise inside the hop window
        const auto coarseStart = (juce::int64) f * hop;
        const auto windowEnd = juce::jmin (totalLen, coarseStart + (juce::int64) std::lround (0.035 * sampleRate));
        float peak = 0.0f;
        for (juce::int64 i = coarseStart; i < windowEnd; ++i)
            peak = juce::jmax (peak, std::abs (mono[(size_t) i]));

        int pitch = options.singleNote;
        if (options.classifyBands)
        {
            // Sum band energy over first 35 ms of hit
            double sumL = 0.0, sumM = 0.0, sumH = 0.0;
            for (juce::int64 i = coarseStart; i < windowEnd; ++i)
            {
                sumL += (double) bLow[(size_t) i] * bLow[(size_t) i];
                sumM += (double) bMid[(size_t) i] * bMid[(size_t) i];
                sumH += (double) bHigh[(size_t) i] * bHigh[(size_t) i];
            }
            const auto tot = juce::jmax (1.0e-12, sumL + sumM + sumH);
            const auto rL = sumL / tot;
            const auto rH = sumH / tot;

            if (rL >= 0.48)
            {
                pitch = 36; // Kick
                ++kicks;
            }
            else if (rH >= 0.35)
            {
                // Open hat if high-band energy still sustains 140 ms later
                const auto tailFrame = juce::jmin (numFrames - 1, f + juce::jmax (1, (int) std::lround (0.14 * sampleRate / hop)));
                const bool open = eHigh[(size_t) tailFrame] > eHigh[(size_t) f] * 0.32f;
                pitch = open ? 46 : 42; // Open Hat / Closed Hat
                ++hats;
            }
            else
            {
                pitch = 38; // Snare
                ++snares;
            }
        }

        ClipNote cn;
        cn.pitch = pitch;
        cn.start = coarseStart;
        cn.length = juce::jmin (hitNoteLen, juce::jmax ((juce::int64) 1, totalLen - coarseStart));
        cn.velocity = juce::jlimit (0.2f, 1.0f, peak / juce::jmax (1.0e-5f, maxEnergy * 1.4f));
        out.notes.push_back (cn);

        if (bpm > 10.0)
        {
            const auto beatPos = ((double) coarseStart / sampleRate) * (bpm / 60.0);
            const auto grid16 = std::round (beatPos * 4.0) / 4.0;
            out.grooveOffsets.push_back (beatPos - grid16);
        }
    }

    if (options.classifyBands)
        out.summary = juce::String (out.notes.size()) + " hits (" + juce::String (kicks) + " kick, "
                    + juce::String (snares) + " snare, " + juce::String (hats) + " hat)";
    else
        out.summary = juce::String (out.notes.size()) + " drum hits extracted";
    return out;
}
}
