#pragma once

#include <JuceHeader.h>

/** The wavetables the built-in synth plays: each a run of single-cycle frames that its Position
    morphs through, kept band-limited for every octave. A frame is made once, at a high resolution,
    and taken into the frequency domain; each octave's copy keeps only the harmonics that stay under
    half the sample rate at the highest pitch it is played at, so no note aliases. Built once for
    the whole app (about 5 MB), on first use, and only read after that. */
namespace WaveTables
{
    enum class Table { analog, pulse, vowel, fm, metal, sub, additive, sync, growl, string, sampled, count };

    /** The tables' names in menu order. "String" is no wavetable: it is a plucked string (each
        unison voice one string), its Position the string's brightness. "Sampled" plays a table made
        from a sound (see Sampled), which each synth holds for itself; until it has one, a sawtooth. */
    const juce::StringArray& names();

    constexpr int numFrames = 32;
    constexpr int numLevels = 11;      // level L keeps 1024 >> L harmonics
    constexpr int maxHarmonics = 1024;

    /** One octave's frames: `size` samples each and one more (the first again) for interpolating. */
    struct Level
    {
        int size = 0;
        int harmonics = 0;
        int frames = numFrames;
        std::vector<float> samples;    // frames * (size + 1)

        const float* frame (int f) const noexcept   { return samples.data() + (size_t) f * (size_t) (size + 1); }
    };

    struct Bank
    {
        std::array<Level, numLevels> levels;
    };

    /** The tables, built on the first call (safe from any thread; the audio thread should never be
        the first). The String table's bank holds a sawtooth-like frame for drawing only. */
    const Bank& get (Table);

    /** The level that keeps a wave with this phase increment (cycles a sample) under Nyquist. */
    inline int levelFor (double increment) noexcept
    {
        // Harmonics allowed: 0.5 / increment; level L has 1024 >> L.
        const auto allowed = 0.5 / juce::jmax (1.0e-9, std::abs (increment));
        int level = 0;

        while (level < numLevels - 1 && (double) (maxHarmonics >> level) > allowed)
            ++level;

        return level;
    }

    /** A sample of a level at `phase` (0 to 1) and `position` (0 to 1 through the frames), each
        between its neighbours. */
    inline float read (const Level& l, float position, double phase) noexcept
    {
        const auto fp = juce::jlimit (0.0f, 1.0f, position) * (float) (l.frames - 1);
        const auto f0 = juce::jmin ((int) fp, l.frames - 2);
        const auto ff = fp - (float) f0;
        const auto x = phase * (double) l.size;
        auto i = (int) x;
        const auto t = (float) (x - (double) i);
        i = juce::jlimit (0, l.size - 1, i);
        const auto* a = l.frame (f0);
        const auto* b = a + (l.size + 1);
        const auto sa = a[i] + t * (a[i + 1] - a[i]);

        if (ff <= 0.0f)
            return sa;

        const auto sb = b[i] + t * (b[i + 1] - b[i]);
        return sa + ff * (sb - sa);
    }

    /** The frame at a position as `points` samples over one cycle, at full bandwidth: for drawing. */
    std::vector<float> drawFrame (Table, float position, int points);

    /** A table made from a sound: its spectrum taken at steps through it, each step one frame, so
        Position plays through the sound's tone from its start to its end. A frame keeps how strong
        each of 256 harmonics of the root is, not their phases (every frame has the same, chosen to
        keep its peaks low), so frames blend into each other without cancelling. The strengths are
        kept as 16-bit steps over 96 dB below each frame's strongest, which is what a saved patch
        holds: a table read back from a patch is the same table, sample for sample. */
    struct Sampled
    {
        static constexpr int harmonics = 256, maxFrames = 64;

        int frames = 0;                    // 2 to maxFrames
        double rootHz = 0.0;               // the pitch whose harmonics the frames hold: played at it, the table has the sound's own spectrum
        juce::String name;
        std::vector<juce::uint16> strengths;   // frames * harmonics
        Bank bank;

        /** The strengths as text for a patch's state, and a table from that text (nullptr if it isn't one). */
        juce::String toText() const;
        static std::shared_ptr<const Sampled> fromText (const juce::String& text, int frames, double rootHz, const juce::String& name);
    };

    /** A table from each frame's harmonic amplitudes (frames * Sampled::harmonics, any scale). */
    std::shared_ptr<const Sampled> makeSampled (const std::vector<float>& amplitudes, int frames, double rootHz, const juce::String& name);

    /** A table from a sound (one channel): up to 64 frames spread evenly through it. `rootHz` is the
        pitch its harmonics are measured at; 0 finds the sound's own pitch, and takes a low root
        (the sample rate / 2048) for a sound without one, which keeps its spectrum in place when
        the table is played at that root. Nullptr for less than a tenth of a second. */
    std::shared_ptr<const Sampled> sampledFromAudio (const float* samples, int numSamples, double sampleRate, double rootHz, const juce::String& name);

    /** The pitch of a sound, by autocorrelation between 30 Hz and 2 kHz; 0 when it has none clear. */
    double findPitch (const float* samples, int numSamples, double sampleRate);

    /** A frame of a bank at a position, for drawing (as drawFrame). */
    std::vector<float> drawFrame (const Bank&, float position, int points);
}
