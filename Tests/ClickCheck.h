#pragma once

#include <JuceHeader.h>

/** The click measure the instruments' tests share: the worst lone jump in a render. A jump between
    two samples of `floor` or more, three times the steps either side of it, is given as a ratio to
    the largest step in the 30 ms around it (the millisecond beside it left out); the tests fail a
    ratio above 4. Jumps within 3 ms of a sample listed in `starts` (a drum hit's own attack) are
    left out. The same measure as the sequence presets' click check (Tests/PresetTests.cpp). */
namespace ClickCheck
{
    struct Click
    {
        float ratio = 0.0f, step = 0.0f;
        double seconds = 0.0;
    };

    inline Click worst (const juce::AudioBuffer<float>& b, double rate, float floor, const std::vector<int>& starts = {})
    {
        Click worst;
        const auto size = juce::jmax (1, (int) (rate / 1000.0));
        const auto blocks = b.getNumSamples() / size;

        for (int ch = 0; ch < b.getNumChannels(); ++ch)
        {
            const auto* x = b.getReadPointer (ch);
            std::vector<float> largest ((size_t) blocks, 0.0f);
            std::vector<int> where ((size_t) blocks, 0);

            for (int i = 1; i < blocks * size; ++i)
                if (const auto step = std::abs (x[i] - x[i - 1]); step > largest[(size_t) (i / size)])
                {
                    largest[(size_t) (i / size)] = step;
                    where[(size_t) (i / size)] = i;
                }

            for (int at = 0; at < blocks; ++at)
            {
                const auto i = where[(size_t) at];

                if (largest[(size_t) at] < floor || i < 2 || i + 1 >= b.getNumSamples()
                    || largest[(size_t) at] < 3.0f * juce::jmax (std::abs (x[i - 1] - x[i - 2]), std::abs (x[i + 1] - x[i])))
                    continue;

                if (std::any_of (starts.begin(), starts.end(), [&] (int start) { return i >= start && i < start + (int) (0.003 * rate); }))
                    continue;

                auto around = 0.0f;

                for (int other = juce::jmax (0, at - 31); other < juce::jmin (blocks, at + 32); ++other)
                    if (std::abs (other - at) > 1)
                        around = juce::jmax (around, largest[(size_t) other]);

                if (const auto ratio = largest[(size_t) at] / juce::jmax (around, 1.0e-6f); ratio > worst.ratio)
                    worst = { ratio, largest[(size_t) at], (double) where[(size_t) at] / rate };
            }
        }

        return worst;
    }
}
