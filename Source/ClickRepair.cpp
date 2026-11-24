#include "ClickRepair.h"
#include "AudioEdits.h"

namespace ClickRepair
{
namespace
{
    constexpr int order = 32;
    constexpr int blockSize = 4096;          // detection blocks, analysed over twice their length
    constexpr int pad = 3;                   // samples repaired either side of a detection, plus a
                                             // quarter of its length (longer clicks fade out below the threshold)
    constexpr int mergeGap = order / 2;      // detections closer than this are one click
    constexpr int pairReach = 8;             // a forward miss and a backward miss this close, in that order, are one click
    constexpr double minSigma = 1.0e-5;      // -100 dBFS: silence is not full of clicks
    constexpr double onsetStep = 3.0;        // the level either side of a click is within this ratio; more is a note beginning or ending
    constexpr int outputChunk = 1 << 16;

    struct Gap
    {
        int channel;
        juce::int64 start, end;
    };

    /** Levinson-Durbin: prediction-error filter a[0..p] (a[0] = 1) from autocorrelation r[0..p]. */
    bool levinson (const std::vector<double>& r, int p, std::vector<double>& a)
    {
        a.assign ((size_t) p + 1, 0.0);
        a[0] = 1.0;
        auto error = r[0] * (1.0 + 1.0e-9);   // a touch of white noise keeps pure tones stable

        if (error <= 0.0)
            return false;

        std::vector<double> previous;

        for (int i = 1; i <= p; ++i)
        {
            auto acc = r[(size_t) i];

            for (int j = 1; j < i; ++j)
                acc += a[(size_t) j] * r[(size_t) (i - j)];

            const auto k = -acc / error;
            previous = a;

            for (int j = 1; j < i; ++j)
                a[(size_t) j] = previous[(size_t) j] + k * previous[(size_t) (i - j)];

            a[(size_t) i] = k;
            error *= 1.0 - k * k;

            if (error <= 0.0)
                return false;
        }

        return true;
    }

    /** Adds the Hann-windowed autocorrelation of x[0..n) at lags 0..p to r. */
    void addAutocorrelation (const float* x, int n, int p, std::vector<double>& r)
    {
        if (n <= p)
            return;

        std::vector<double> w ((size_t) n);

        for (int i = 0; i < n; ++i)
            w[(size_t) i] = x[i] * 0.5 * (1.0 - std::cos (juce::MathConstants<double>::twoPi * (i + 0.5) / n));

        for (int lag = 0; lag <= p; ++lag)
        {
            double sum = 0.0;

            for (int i = lag; i < n; ++i)
                sum += w[(size_t) i] * w[(size_t) (i - lag)];

            r[(size_t) lag] += sum;
        }
    }

    /** Solves the symmetric positive definite system m x = b in place (Cholesky). */
    bool solve (std::vector<double>& m, std::vector<double>& b, int n)
    {
        for (int j = 0; j < n; ++j)
        {
            auto d = m[(size_t) (j * n + j)];

            for (int k = 0; k < j; ++k)
                d -= m[(size_t) (j * n + k)] * m[(size_t) (j * n + k)];

            if (d <= 0.0)
                return false;

            d = std::sqrt (d);
            m[(size_t) (j * n + j)] = d;

            for (int i = j + 1; i < n; ++i)
            {
                auto s = m[(size_t) (i * n + j)];

                for (int k = 0; k < j; ++k)
                    s -= m[(size_t) (i * n + k)] * m[(size_t) (j * n + k)];

                m[(size_t) (i * n + j)] = s / d;
            }
        }

        for (int i = 0; i < n; ++i)
        {
            auto s = b[(size_t) i];

            for (int k = 0; k < i; ++k)
                s -= m[(size_t) (i * n + k)] * b[(size_t) k];

            b[(size_t) i] = s / m[(size_t) (i * n + i)];
        }

        for (int i = n - 1; i >= 0; --i)
        {
            auto s = b[(size_t) i];

            for (int k = i + 1; k < n; ++k)
                s -= m[(size_t) (k * n + i)] * b[(size_t) k];

            b[(size_t) i] = s / m[(size_t) (i * n + i)];
        }

        return true;
    }

    /** Clicks starting in [from, to), per channel, in order. A click's core is where both predictions
        miss; it reaches back to the first forward miss before the core (forward prediction from clean
        audio fails where the click begins) and on to the last backward miss after it (backward
        prediction fails where it ends), so smooth click edges are caught as well. */
    void detectBlock (const AudioSnapshot& snapshot, juce::int64 from, juce::int64 to, double threshold, bool voice, int stretch,
                      juce::AudioBuffer<float>& window, std::vector<Gap>& raw)
    {
        const auto windowStart = from - blockSize / 2;
        snapshot.read (window, 0, windowStart, window.getNumSamples());
        const auto count = (int) (to - from);
        const auto offset = blockSize / 2;
        const auto span = count + 2 * order;   // errors also for order samples either side of the block
        std::vector<double> r ((size_t) order + 1), a, forward ((size_t) span), backward ((size_t) span), sorted, limits ((size_t) span), spreads;

        for (int ch = 0; ch < window.getNumChannels(); ++ch)
        {
            const auto* x = window.getReadPointer (ch);
            std::fill (r.begin(), r.end(), 0.0);
            addAutocorrelation (x, window.getNumSamples(), order, r);

            if (! levinson (r, order, a))
                continue;

            // Index j holds the errors of sample from + j - order.
            for (int j = 0; j < span; ++j)
            {
                const auto i = offset + j - order;
                double f = 0.0, b = 0.0;

                for (int k = 0; k <= order; ++k)
                {
                    f += a[(size_t) k] * x[i - k];
                    b += a[(size_t) k] * x[i + k];
                }

                forward[(size_t) j] = std::abs (f);
                backward[(size_t) j] = std::abs (b);
            }

            // Robust spread of the forward error over the block: median absolute value / 0.6745; for a
            // voice, the 95th percentile / 1.96 where that is more (the same for a Gaussian error; more
            // where the error is spiky of itself, as a voice's own pulses make it, which are not clicks).
            sorted.assign (forward.begin() + order, forward.begin() + order + count);
            std::nth_element (sorted.begin(), sorted.begin() + count / 2, sorted.end());
            auto spread = sorted[(size_t) count / 2] / 0.6745;

            if (voice)
            {
                const auto high = juce::jmin (count - 1, (int) (0.95 * count));
                std::nth_element (sorted.begin(), sorted.begin() + high, sorted.end());
                spread = juce::jmax (spread, sorted[(size_t) high] / 1.96);
            }

            // The same spread over each stretch of the block (stretch samples, several times a click's
            // longest). A sample's limit is over the block's spread and over its own stretch's and the
            // stretches' either side, so where the error is large of itself for a while, as through a
            // drum hit or a plucked note's attack, it is not a row of clicks.
            const auto stretches = (span + stretch - 1) / stretch;
            spreads.assign ((size_t) stretches, 0.0);

            for (int s = 0; s < stretches; ++s)
            {
                const auto begin = s * stretch, n = juce::jmin (stretch, span - begin);
                sorted.assign (forward.begin() + begin, forward.begin() + begin + n);
                std::nth_element (sorted.begin(), sorted.begin() + n / 2, sorted.end());
                spreads[(size_t) s] = sorted[(size_t) n / 2] / 0.6745;
            }

            for (int j = 0; j < span; ++j)
            {
                const auto s = j / stretch;
                const auto local = juce::jmax (spreads[(size_t) juce::jmax (0, s - 1)], spreads[(size_t) s], spreads[(size_t) juce::jmin (stretches - 1, s + 1)]);
                limits[(size_t) j] = threshold * juce::jmax (minSigma, spread, local);
            }

            auto both = [&] (int j) { return forward[(size_t) j] > limits[(size_t) j] && backward[(size_t) j] > limits[(size_t) j]; };

            // A core starts where both miss at one sample, or where the forward prediction misses and
            // the backward one misses within pairReach samples after: a click of a few samples at one
            // level (a flat top) is missed forward at its first sample and backward at its last, and
            // need not be missed both ways at any one of them. (A sound's own attack is the other way
            // round: the backward miss comes before the forward one.)
            auto pairedEnd = [&] (int j)
            {
                if (forward[(size_t) j] <= limits[(size_t) j])
                    return -1;

                for (int k = juce::jmin (span - 1, j + pairReach); k >= j; --k)
                    if (backward[(size_t) k] > limits[(size_t) k])
                        return k;

                return -1;
            };

            for (int j = order; j < order + count;)
            {
                const auto paired = pairedEnd (j);

                if (paired < 0)
                {
                    ++j;
                    continue;
                }

                auto coreEnd = paired + 1;

                while (coreEnd < span && both (coreEnd))
                    ++coreEnd;

                auto first = j;

                for (int k = j - 1; k >= j - order; --k)
                    if (forward[(size_t) k] > limits[(size_t) k])
                        first = k;

                auto last = coreEnd - 1;

                for (int k = coreEnd; k < juce::jmin (span, coreEnd + order); ++k)
                    if (backward[(size_t) k] > limits[(size_t) k])
                        last = k;

                // A click passes: the sound after it is about as loud as the sound before. Where the
                // level steps (by more than onsetStep, over a stretch either side), a note or a hit
                // begins or ends there, and that is the sound's own.
                const auto levelOf = [x] (int begin, int n)
                {
                    auto sum = 0.0;

                    for (int i = begin; i < begin + n; ++i)
                        sum += (double) x[i] * x[i];

                    return std::sqrt (sum / n);
                };

                const auto reach = juce::jmin (stretch, offset - order - 1);
                const auto before = levelOf (offset + first - order - reach, reach), after = levelOf (offset + last + 1 - order, reach);

                if (juce::jmax (before, after) <= onsetStep * juce::jmax (minSigma, juce::jmin (before, after)))
                    raw.push_back ({ ch, from + first - order, from + last + 1 - order });

                j = juce::jmax (coreEnd, last + 1);
            }
        }
    }

    /** Least-squares AR interpolation of [gap.start, gap.end) from the audio either side. */
    std::optional<std::vector<float>> interpolate (const AudioSnapshot& snapshot, const Gap& gap)
    {
        const auto m = (int) (gap.end - gap.start);
        const auto context = juce::jmax (2048, 8 * m);   // enough periods to resolve low tones
        const auto left = (int) juce::jmin ((juce::int64) context, gap.start);
        const auto right = (int) juce::jmin ((juce::int64) context, snapshot.getNumFrames() - gap.end);

        if (left < 2 * order || right < 2 * order)
            return std::nullopt;

        juce::AudioBuffer<float> all (snapshot.getNumChannels(), left + m + right);
        snapshot.read (all, 0, gap.start - left, all.getNumSamples());
        std::vector<float> x (all.getReadPointer (gap.channel), all.getReadPointer (gap.channel) + all.getNumSamples());

        std::vector<double> r ((size_t) order + 1), a;
        addAutocorrelation (x.data(), left, order, r);
        addAutocorrelation (x.data() + left + m, right, order, r);

        if (! levinson (r, order, a))
            return std::nullopt;

        // Prediction error over every equation that touches the gap, with the gap set to zero.
        std::fill (x.begin() + left, x.begin() + left + m, 0.0f);
        std::vector<double> e0 ((size_t) (m + order));

        for (int t = 0; t < m + order; ++t)
        {
            double e = 0.0;

            for (int k = 0; k <= order; ++k)
                e += a[(size_t) k] * x[(size_t) (left + t - k)];

            e0[(size_t) t] = e;
        }

        // Normal equations: the matrix is Toeplitz in the filter's autocorrelation.
        std::vector<double> ra ((size_t) order + 1);

        for (int d = 0; d <= order; ++d)
            for (int k = 0; k + d <= order; ++k)
                ra[(size_t) d] += a[(size_t) k] * a[(size_t) (k + d)];

        std::vector<double> matrix ((size_t) (m * m)), b ((size_t) m);

        for (int i = 0; i < m; ++i)
        {
            for (int j = 0; j < m; ++j)
                matrix[(size_t) (i * m + j)] = std::abs (i - j) <= order ? ra[(size_t) std::abs (i - j)] : 0.0;

            double s = 0.0;

            for (int k = 0; k <= order; ++k)
                s += a[(size_t) k] * e0[(size_t) (i + k)];

            b[(size_t) i] = -s;
        }

        if (! solve (matrix, b, m))
            return std::nullopt;

        return std::vector<float> (b.begin(), b.end());
    }
}

namespace
{
    /** Solves the symmetric positive definite banded system m u = b in place (Cholesky): row i of
        m holds columns i - band .. i, the diagonal last. */
    bool solveBanded (std::vector<double>& m, std::vector<double>& b, int n, int band)
    {
        const auto w = band + 1;
        const auto at = [&m, w, band] (int i, int j) -> double& { return m[(size_t) (i * w + (j - i + band))]; };

        for (int i = 0; i < n; ++i)
        {
            for (int j = juce::jmax (0, i - band); j <= i; ++j)
            {
                auto sum = at (i, j);

                for (int k = juce::jmax (0, i - band); k < j; ++k)
                    sum -= at (i, k) * at (j, k);

                if (j < i)
                {
                    at (i, j) = sum / at (j, j);
                }
                else
                {
                    if (sum <= 0.0)
                        return false;

                    at (i, i) = std::sqrt (sum);
                }
            }
        }

        for (int i = 0; i < n; ++i)
        {
            auto sum = b[(size_t) i];

            for (int k = juce::jmax (0, i - band); k < i; ++k)
                sum -= at (i, k) * b[(size_t) k];

            b[(size_t) i] = sum / at (i, i);
        }

        for (int i = n - 1; i >= 0; --i)
        {
            auto sum = b[(size_t) i];

            for (int k = i + 1; k <= juce::jmin (n - 1, i + band); ++k)
                sum -= at (k, i) * b[(size_t) k];

            b[(size_t) i] = sum / at (i, i);
        }

        return true;
    }
}

bool fillMissing (float* x, int n, const std::vector<int>& missing, const std::vector<float>* beyond)
{
    if (missing.empty() || missing.front() < order || missing.back() + order >= n)
        return false;

    std::vector<double> r ((size_t) order + 1), a;
    addAutocorrelation (x, n, order, r);

    if (! levinson (r, order, a))
        return false;

    std::vector<double> ra ((size_t) order + 1);

    for (int d = 0; d <= order; ++d)
        for (int k = 0; k + d <= order; ++k)
            ra[(size_t) d] += a[(size_t) k] * a[(size_t) (k + d)];

    // The samples still to solve for (`open`), and a copy of x with them at zero; a sample set to
    // its bound leaves `open` and stays in the copy as known.
    std::vector<double> work (x, x + n);
    std::vector<int> open ((size_t) missing.size());
    std::iota (open.begin(), open.end(), 0);

    for (int round = 0; round < 4 && ! open.empty(); ++round)
    {
        const auto m = (int) open.size();
        const auto first = missing[(size_t) open.front()], last = missing[(size_t) open.back()];

        for (auto i : open)
            work[(size_t) missing[(size_t) i]] = 0.0;

        // Prediction error over every equation that touches an open sample, with those at zero.
        std::vector<double> e0 ((size_t) (last - first + order + 1));

        for (int t = first; t <= last + order; ++t)
        {
            auto e = 0.0;

            for (int k = 0; k <= order; ++k)
                e += a[(size_t) k] * work[(size_t) (t - k)];

            e0[(size_t) (t - first)] = e;
        }

        // Normal equations: banded, since samples more than `order` apart share no equation.
        const auto w = order + 1;
        std::vector<double> matrix ((size_t) (m * w)), b ((size_t) m);

        for (int i = 0; i < m; ++i)
        {
            const auto pi = missing[(size_t) open[(size_t) i]];

            for (int j = juce::jmax (0, i - order); j <= i; ++j)
            {
                const auto d = pi - missing[(size_t) open[(size_t) j]];
                matrix[(size_t) (i * w + (j - i + order))] = d <= order ? ra[(size_t) d] * (i == j ? 1.0 + 1.0e-9 : 1.0) : 0.0;
            }

            auto sum = 0.0;

            for (int k = 0; k <= order; ++k)
                sum += a[(size_t) k] * e0[(size_t) (pi + k - first)];

            b[(size_t) i] = -sum;
        }

        if (! solveBanded (matrix, b, m, order))
            return false;

        std::vector<int> still;

        for (int i = 0; i < m; ++i)
        {
            const auto index = open[(size_t) i];
            const auto bound = beyond != nullptr ? (double) (*beyond)[(size_t) index] : 0.0;
            const auto shortOfIt = beyond != nullptr && (bound >= 0.0 ? b[(size_t) i] < bound : b[(size_t) i] > bound);
            work[(size_t) missing[(size_t) index]] = shortOfIt ? bound : b[(size_t) i];

            if (! shortOfIt)
                still.push_back (index);
        }

        if (still.size() == open.size())
            break;

        open = std::move (still);
    }

    for (auto p : missing)
        x[p] = (float) work[(size_t) p];

    return true;
}

std::optional<Result> repair (const AudioSnapshot& snapshot, juce::int64 start, juce::int64 end, double sampleRate,
                              Settings settings, const std::function<bool (float)>& progress)
{
    const auto length = snapshot.getNumFrames();
    start = juce::jlimit ((juce::int64) 0, length, start);
    end = juce::jlimit (start, length, end);
    Result result;

    if (end <= start)
        return result;

    // 1. Detection, block by block (85% of the progress bar).
    const auto threshold = std::pow (10.0, settings.thresholdDb / 20.0);
    juce::AudioBuffer<float> window (snapshot.getNumChannels(), 2 * blockSize);
    std::vector<Gap> raw;

    // A stretch for the local spread: at least three and a half times the longest click, so a click
    // is never most of one (512 samples at 48 kHz with the default 3 ms).
    const auto longest = juce::jmax (1.0, settings.maxLengthMs / 1000.0 * sampleRate);
    const auto stretch = juce::jlimit (64, blockSize, juce::nextPowerOfTwo ((int) std::ceil (3.5 * longest)));

    for (auto b = start; b < end; b += blockSize)
    {
        detectBlock (snapshot, b, juce::jmin (end, b + blockSize), threshold, settings.voice, stretch, window, raw);

        if (progress != nullptr && ! progress (0.85f * (float) ((double) (b - start) / (double) (end - start))))
            return std::nullopt;
    }

    // 2. Margins and merging, per channel, kept inside the range.
    std::stable_sort (raw.begin(), raw.end(), [] (const Gap& x, const Gap& y)
                      { return x.channel != y.channel ? x.channel < y.channel : x.start < y.start; });
    std::vector<Gap> gaps;

    for (auto g : raw)
    {
        const auto margin = pad + (g.end - g.start) / 4;
        g.start = juce::jmax (start, g.start - margin);
        g.end = juce::jmin (end, g.end + margin);

        if (! gaps.empty() && gaps.back().channel == g.channel && g.start - gaps.back().end <= mergeGap)
            gaps.back().end = juce::jmax (gaps.back().end, g.end);
        else
            gaps.push_back (g);
    }

    // 3. Interpolation, from the original audio around each click.
    const auto maxLength = (juce::int64) std::llround (settings.maxLengthMs / 1000.0 * sampleRate);
    std::vector<std::pair<Gap, std::vector<float>>> fixes;

    for (size_t i = 0; i < gaps.size(); ++i)
    {
        const auto& g = gaps[i];
        auto fixed = g.end - g.start <= maxLength ? interpolate (snapshot, g) : std::nullopt;

        if (fixed.has_value())
            fixes.emplace_back (g, std::move (*fixed));
        else
            ++result.skipped;

        if (progress != nullptr && (i & 15) == 0 && ! progress (0.85f + 0.15f * (float) i / (float) gaps.size()))
            return std::nullopt;
    }

    if (fixes.empty())
        return result;

    // Clicks are counted once across channels.
    std::vector<juce::Range<juce::int64>> spans;

    for (auto& f : fixes)
        spans.push_back ({ f.first.start, f.first.end });

    std::sort (spans.begin(), spans.end(), [] (auto& x, auto& y) { return x.getStart() < y.getStart(); });

    for (size_t i = 0; i < spans.size(); ++result.repaired)
    {
        auto reach = spans[i].getEnd();

        for (++i; i < spans.size() && spans[i].getStart() < reach; ++i)
            reach = juce::jmax (reach, spans[i].getEnd());
    }

    // 4. Only the stretches with repairs are rewritten; the rest keeps sharing the old blocks.
    std::sort (fixes.begin(), fixes.end(), [] (auto& x, auto& y) { return x.first.start < y.first.start; });
    auto current = std::make_shared<const AudioSnapshot> (snapshot);

    for (size_t i = 0; i < fixes.size();)
    {
        const auto chunkStart = start + (fixes[i].first.start - start) / outputChunk * outputChunk;
        auto chunkEnd = chunkStart + outputChunk;
        auto last = i;

        while (last < fixes.size() && fixes[last].first.start < chunkEnd)
            chunkEnd = juce::jmax (chunkEnd, fixes[last++].first.end);

        chunkEnd = juce::jmin (chunkEnd, end);
        current = AudioEdits::processRange (*current, chunkStart, chunkEnd, [&] (juce::AudioBuffer<float>& b)
        {
            for (auto f = i; f < last; ++f)
            {
                const auto& [gap, samples] = fixes[f];
                b.copyFrom (gap.channel, (int) (gap.start - chunkStart), samples.data(), (int) samples.size());
            }
        });
        i = last;
    }

    result.snapshot = current;
    return result;
}
}
