#include "InstrumentKeys.h"

/** The audition patterns: for each kind of sound a menu of phrases at 125 BPM, the first of each
    the phrase it had before there was a choice. Notes are written for the keys at octave 3. */
namespace InstrumentKeys
{
namespace
{
    using Notes = std::vector<PhraseNote>;

    constexpr double sixteenth = 120.0;          // at 125 BPM
    constexpr double bar = 16 * sixteenth;

    /** A note on the sixteenths' grid: its step, how many steps it is held, semitones from the root, how hard. */
    struct Step
    {
        int step, length, semitones;
        float velocity;
        bool slide = false;     // held into the next note, for a patch's glide
    };

    Notes line (int root, std::initializer_list<Step> steps, double stepMs = sixteenth)
    {
        Notes out;

        for (auto& s : steps)
            out.push_back ({ root + s.semitones, s.step * stepMs, s.length * stepMs + (s.slide ? 30.0 : -10.0), s.velocity });

        return out;
    }

    Notes chord (std::initializer_list<int> notes, int step, int length, float velocity)
    {
        Notes out;

        for (auto n : notes)
            out.push_back ({ n, step * sixteenth, length * sixteenth - 10.0, velocity });

        return out;
    }

    Notes join (std::initializer_list<Notes> parts)
    {
        Notes out;

        for (auto& p : parts)
            out.insert (out.end(), p.begin(), p.end());

        return out;
    }

    /** The same chord on each of `steps`, `length` steps long. */
    Notes chords (std::initializer_list<int> notes, std::initializer_list<int> steps, int length, float velocity)
    {
        Notes out;

        for (auto s : steps)
        {
            const auto one = chord (notes, s, length, velocity);
            out.insert (out.end(), one.begin(), one.end());
        }

        return out;
    }

    /** A drum hit on the grid. */
    Notes hits (int note, std::initializer_list<int> steps, float velocity, float offbeatVelocity = -1.0f)
    {
        Notes out;

        for (auto s : steps)
            out.push_back ({ note, s * sixteenth, 60.0, offbeatVelocity >= 0.0f && s % 2 == 1 ? offbeatVelocity : velocity });

        return out;
    }

    std::vector<Pattern> basses()
    {
        constexpr int c = 36;
        return {
            { "Root and Octave", { { 36, 0.0, 100.0, 0.9f }, { 36, 120.0, 100.0, 0.7f }, { 48, 240.0, 100.0, 0.8f }, { 36, 360.0, 220.0, 0.9f } }, 0.0 },
            { "Rolling 16ths", line (c, { { 0, 1, 0, 1.0f }, { 1, 1, 0, 0.6f }, { 2, 1, 0, 0.75f }, { 3, 1, 0, 0.6f }, { 4, 1, 0, 0.95f }, { 5, 1, 0, 0.6f }, { 6, 1, 0, 0.75f }, { 7, 1, 0, 0.6f },
                                          { 8, 1, 0, 0.95f }, { 9, 1, 0, 0.6f }, { 10, 1, 0, 0.75f }, { 11, 1, 0, 0.6f }, { 12, 1, 0, 0.95f }, { 13, 1, 0, 0.6f }, { 14, 1, 3, 0.8f }, { 15, 1, 0, 0.65f } }), bar },
            { "Offbeat 8ths", line (c, { { 2, 2, 0, 0.9f }, { 6, 2, 0, 0.85f }, { 10, 2, 0, 0.9f }, { 14, 2, 0, 0.85f } }), bar },
            { "Iron Gallop", line (c, { { 0, 2, 0, 1.0f }, { 2, 1, 0, 0.7f }, { 3, 1, 0, 0.75f }, { 4, 2, 0, 0.95f }, { 6, 1, 0, 0.7f }, { 7, 1, 0, 0.75f },
                                       { 8, 2, 0, 0.95f }, { 10, 1, 0, 0.7f }, { 11, 1, 0, 0.75f }, { 12, 2, 0, 0.95f }, { 14, 1, 0, 0.7f }, { 15, 1, 0, 0.75f },
                                       { 16, 2, 0, 1.0f }, { 18, 1, 0, 0.7f }, { 19, 1, 0, 0.75f }, { 20, 2, 0, 0.95f }, { 22, 1, 0, 0.7f }, { 23, 1, 0, 0.75f },
                                       { 24, 2, 3, 0.95f }, { 26, 1, 3, 0.7f }, { 27, 1, 3, 0.75f }, { 28, 2, -2, 0.95f }, { 30, 1, -2, 0.7f }, { 31, 1, -2, 0.75f } }), 2 * bar },
            { "Driving 8ths", line (c, { { 0, 2, 0, 1.0f }, { 2, 2, 0, 0.75f }, { 4, 2, 0, 0.9f }, { 6, 2, 0, 0.75f }, { 8, 2, 0, 0.9f }, { 10, 2, 0, 0.75f }, { 12, 2, 0, 0.9f }, { 14, 2, 12, 0.8f } }), bar },
            { "Broken Techno", line (c, { { 0, 2, 0, 1.0f }, { 3, 2, 0, 0.8f }, { 6, 1, 12, 0.85f }, { 10, 2, 0, 0.9f }, { 12, 1, 3, 0.8f }, { 14, 2, 0, 0.85f } }), bar },
            // Slides: a note held into the next of another pitch, for a patch with glide; accents by velocity.
            { "Acid Line", line (c, { { 0, 1, 0, 1.0f }, { 1, 1, 0, 0.6f }, { 2, 2, 12, 0.95f, true }, { 4, 1, 0, 0.6f }, { 5, 2, 3, 0.7f, true }, { 7, 1, 15, 1.0f }, { 8, 1, 0, 0.6f }, { 9, 1, 0, 0.65f },
                                      { 10, 2, 10, 0.95f, true }, { 12, 1, 0, 0.6f }, { 13, 2, 7, 0.7f, true }, { 15, 1, 12, 1.0f },
                                      { 16, 1, 0, 1.0f }, { 17, 1, 0, 0.6f }, { 18, 2, 3, 0.7f, true }, { 20, 1, 15, 1.0f }, { 21, 1, 0, 0.6f }, { 22, 2, 12, 0.95f, true }, { 24, 1, 0, 0.6f }, { 25, 1, 5, 0.7f },
                                      { 26, 2, 6, 0.95f, true }, { 28, 1, 0, 0.6f }, { 29, 2, 10, 0.7f, true }, { 31, 1, 12, 1.0f } }), 2 * bar },
            { "Octave Jumps", line (c, { { 0, 2, 0, 0.95f }, { 2, 2, 12, 0.8f }, { 4, 2, 0, 0.9f }, { 6, 2, 12, 0.8f }, { 8, 2, 0, 0.9f }, { 10, 2, 12, 0.8f }, { 12, 2, 0, 0.9f }, { 14, 2, 12, 0.8f } }), bar },
            { "Long Notes", line (c, { { 0, 16, 0, 0.9f }, { 16, 16, 3, 0.9f } }), 2 * bar },
            { "Half-Time", line (c, { { 0, 6, 0, 1.0f }, { 10, 2, 0, 0.8f }, { 16, 6, 0, 1.0f }, { 26, 4, -2, 0.85f } }), 2 * bar },
            { "Syncopated", line (c, { { 0, 2, 0, 1.0f }, { 3, 2, 7, 0.8f }, { 6, 2, 0, 0.9f }, { 9, 2, 10, 0.8f }, { 12, 1, 0, 0.85f }, { 14, 2, 12, 0.8f } }), bar },
            { "Fast Trill", line (c, { { 0, 1, 0, 1.0f }, { 1, 1, 1, 0.7f }, { 2, 1, 0, 0.8f }, { 3, 1, 1, 0.7f }, { 4, 1, 0, 0.9f }, { 5, 1, 1, 0.7f }, { 6, 1, 0, 0.8f }, { 7, 1, 1, 0.7f },
                                      { 8, 1, 0, 0.9f }, { 9, 1, 1, 0.7f }, { 10, 1, 0, 0.8f }, { 11, 1, 1, 0.7f }, { 12, 1, 0, 0.9f }, { 13, 1, 1, 0.7f }, { 14, 1, 0, 0.8f }, { 15, 1, 1, 0.7f },
                                      { 16, 1, 0, 0.9f }, { 17, 1, 1, 0.7f }, { 18, 1, 0, 0.8f }, { 19, 1, 1, 0.7f }, { 20, 1, 0, 0.9f }, { 21, 1, 1, 0.7f }, { 22, 1, 0, 0.8f }, { 23, 1, 1, 0.7f },
                                      { 24, 1, 0, 0.9f }, { 25, 1, 1, 0.7f }, { 26, 1, 0, 0.8f }, { 27, 1, 1, 0.7f }, { 28, 1, 0, 0.9f }, { 29, 1, 1, 0.7f }, { 30, 1, 0, 0.8f }, { 31, 1, 1, 0.7f } },
                                 sixteenth / 2.0), bar },
        };
    }

    std::vector<Pattern> chordSounds()
    {
        return {
            { "Held Chord", { { 48, 0.0, 1600.0, 0.75f }, { 55, 0.0, 1600.0, 0.75f }, { 60, 0.0, 1600.0, 0.75f }, { 63, 0.0, 1600.0, 0.75f } }, 0.0 },
            { "Minor Progression", join ({ chord ({ 48, 55, 60, 63 }, 0, 15, 0.75f), chord ({ 44, 51, 56, 60 }, 16, 15, 0.75f),
                                           chord ({ 51, 55, 58, 63 }, 32, 15, 0.75f), chord ({ 46, 53, 58, 62 }, 48, 15, 0.75f) }), 4 * bar },
            { "Two Chords", join ({ chord ({ 48, 55, 60, 63 }, 0, 15, 0.75f), chord ({ 41, 48, 53, 56 }, 16, 15, 0.75f) }), 2 * bar },
            { "Long Swell", chord ({ 48, 55, 60, 63 }, 0, 63, 0.6f), 4 * bar },
            { "Dark Fifth", chord ({ 36, 43, 48 }, 0, 31, 0.75f), 2 * bar },
            { "Suspended", join ({ chord ({ 48, 50, 55, 60 }, 0, 15, 0.75f), chord ({ 48, 53, 55, 60 }, 16, 15, 0.75f) }), 2 * bar },
            { "Pulsing 8ths", chords ({ 48, 55, 60, 63 }, { 0, 2, 4, 6, 8, 10, 12, 14 }, 1, 0.7f), bar },
            { "Dub Offbeats", chords ({ 51, 55, 58, 62 }, { 2, 6, 10, 14 }, 1, 0.75f), bar },
            { "High Voicing", chord ({ 60, 67, 70, 75 }, 0, 31, 0.7f), 2 * bar },
            { "Low and Wide", chord ({ 36, 48, 55, 63 }, 0, 31, 0.75f), 2 * bar },
            { "Falling", join ({ chord ({ 48, 55, 60, 63 }, 0, 15, 0.75f), chord ({ 46, 53, 58, 62 }, 16, 15, 0.75f),
                                 chord ({ 44, 51, 56, 60 }, 32, 15, 0.75f), chord ({ 43, 50, 55, 59 }, 48, 15, 0.75f) }), 4 * bar },
            { "Single Note", line (60, { { 0, 31, 0, 0.75f } }), 2 * bar },
        };
    }

    std::vector<Pattern> stabs()
    {
        const auto cm = std::initializer_list<int> { 48, 60, 63, 67 };
        return {
            { "Two Stabs", { { 48, 0.0, 160.0, 0.9f }, { 60, 0.0, 160.0, 0.9f }, { 63, 0.0, 160.0, 0.9f }, { 67, 0.0, 160.0, 0.9f },
                             { 48, 360.0, 260.0, 0.8f }, { 60, 360.0, 260.0, 0.8f }, { 63, 360.0, 260.0, 0.8f }, { 67, 360.0, 260.0, 0.8f } }, 0.0 },
            { "Offbeats", chords (cm, { 2, 6, 10, 14 }, 1, 0.85f), bar },
            { "Dub Stab", chords ({ 51, 55, 58, 62 }, { 0, 6, 12 }, 2, 0.85f), bar },
            { "Rave Stabs", join ({ chords (cm, { 0, 3, 6, 10, 12 }, 1, 0.9f), chords ({ 44, 56, 60, 63 }, { 16, 19, 22, 26, 28 }, 1, 0.9f) }), 2 * bar },
            { "One a Bar", join ({ chord (cm, 0, 2, 0.9f), chord ({ 44, 56, 60, 63 }, 16, 2, 0.9f) }), 2 * bar },
            { "Syncopated", chords (cm, { 0, 3, 6, 8, 11, 14 }, 1, 0.85f), bar },
            { "Chord Change", join ({ chords (cm, { 2, 6, 10, 14 }, 1, 0.85f), chords ({ 41, 53, 56, 60 }, { 18, 22, 26, 30 }, 1, 0.85f) }), 2 * bar },
            { "8ths", chords (cm, { 0, 2, 4, 6, 8, 10, 12, 14 }, 1, 0.8f), bar },
            { "Pushed", chords (cm, { 3, 7, 11, 15 }, 1, 0.85f), bar },
            { "High Stabs", chords ({ 60, 63, 67, 72 }, { 2, 6, 10, 14 }, 1, 0.85f), bar },
            { "Ninth Hit", chord ({ 48, 58, 62, 63, 67 }, 0, 4, 0.9f), bar },
            { "Stab and Hold", join ({ chord (cm, 0, 1, 0.9f), chord (cm, 3, 1, 0.8f), chord (cm, 6, 10, 0.85f) }), bar },
        };
    }

    std::vector<Pattern> leads()
    {
        return {
            { "Rising Figure", { { 60, 0.0, 150.0, 0.85f }, { 63, 170.0, 150.0, 0.75f }, { 67, 340.0, 150.0, 0.8f }, { 72, 510.0, 420.0, 0.9f } }, 0.0 },
            { "Minor Arp", line (60, { { 0, 1, 0, 0.9f }, { 1, 1, 3, 0.7f }, { 2, 1, 7, 0.75f }, { 3, 1, 12, 0.8f }, { 4, 1, 0, 0.85f }, { 5, 1, 3, 0.7f }, { 6, 1, 7, 0.75f }, { 7, 1, 12, 0.8f },
                                       { 8, 1, 0, 0.85f }, { 9, 1, 3, 0.7f }, { 10, 1, 7, 0.75f }, { 11, 1, 12, 0.8f }, { 12, 1, 0, 0.85f }, { 13, 1, 3, 0.7f }, { 14, 1, 7, 0.75f }, { 15, 1, 12, 0.8f } }), bar },
            { "Arp Up and Down", line (60, { { 0, 1, 0, 0.9f }, { 1, 1, 3, 0.7f }, { 2, 1, 7, 0.75f }, { 3, 1, 12, 0.8f }, { 4, 1, 15, 0.85f }, { 5, 1, 12, 0.7f }, { 6, 1, 7, 0.75f }, { 7, 1, 3, 0.7f },
                                             { 8, 1, 0, 0.9f }, { 9, 1, 3, 0.7f }, { 10, 1, 7, 0.75f }, { 11, 1, 12, 0.8f }, { 12, 1, 15, 0.85f }, { 13, 1, 12, 0.7f }, { 14, 1, 7, 0.75f }, { 15, 1, 3, 0.7f } }), bar },
            { "Octave Arp", line (48, { { 0, 1, 0, 0.9f }, { 1, 1, 12, 0.75f }, { 2, 1, 0, 0.8f }, { 3, 1, 15, 0.75f }, { 4, 1, 0, 0.85f }, { 5, 1, 12, 0.75f }, { 6, 1, 0, 0.8f }, { 7, 1, 19, 0.75f },
                                        { 8, 1, 0, 0.85f }, { 9, 1, 12, 0.75f }, { 10, 1, 0, 0.8f }, { 11, 1, 15, 0.75f }, { 12, 1, -2, 0.85f }, { 13, 1, 10, 0.75f }, { 14, 1, -2, 0.8f }, { 15, 1, 14, 0.75f } }), bar },
            { "Melody", line (60, { { 0, 2, 12, 0.9f }, { 2, 2, 10, 0.8f }, { 4, 4, 7, 0.85f }, { 8, 2, 3, 0.8f }, { 10, 2, 5, 0.8f }, { 12, 4, 7, 0.85f },
                                    { 16, 2, 12, 0.9f }, { 18, 2, 15, 0.85f }, { 20, 4, 14, 0.85f }, { 24, 2, 10, 0.8f }, { 26, 2, 7, 0.8f }, { 28, 4, 3, 0.85f } }), 2 * bar },
            { "Call and Answer", line (60, { { 0, 1, 0, 0.9f }, { 2, 1, 3, 0.8f }, { 3, 3, 7, 0.85f }, { 8, 1, 10, 0.85f }, { 10, 1, 7, 0.8f }, { 11, 3, 3, 0.85f },
                                             { 16, 1, 0, 0.9f }, { 18, 1, 3, 0.8f }, { 19, 3, 7, 0.85f }, { 24, 1, 5, 0.85f }, { 26, 1, 3, 0.8f }, { 27, 5, 0, 0.85f } }), 2 * bar },
            { "Long Note", line (72, { { 0, 31, 0, 0.85f } }), 2 * bar },
            { "Gated 16ths", line (72, { { 0, 1, 0, 0.95f }, { 1, 1, 0, 0.65f }, { 2, 1, 0, 0.8f }, { 3, 1, 0, 0.65f }, { 4, 1, 0, 0.9f }, { 5, 1, 0, 0.65f }, { 6, 1, 0, 0.8f }, { 7, 1, 0, 0.65f },
                                         { 8, 1, -2, 0.9f }, { 9, 1, -2, 0.65f }, { 10, 1, -2, 0.8f }, { 11, 1, -2, 0.65f }, { 12, 1, -5, 0.9f }, { 13, 1, -5, 0.65f }, { 14, 1, -5, 0.8f }, { 15, 1, -5, 0.65f } }), bar },
            { "Signal Riff", line (60, { { 0, 2, 0, 0.95f }, { 2, 2, 0, 0.75f }, { 4, 2, 3, 0.85f }, { 6, 2, 0, 0.75f }, { 8, 2, 7, 0.9f }, { 10, 2, 0, 0.75f }, { 12, 2, 5, 0.85f }, { 14, 2, 3, 0.8f } }), bar },
            { "Slow Phrase", line (60, { { 0, 4, 0, 0.85f }, { 4, 4, 3, 0.8f }, { 8, 4, 7, 0.85f }, { 12, 4, 10, 0.8f }, { 16, 16, 12, 0.9f } }), 2 * bar },
            { "Triplet Arp", line (60, { { 0, 1, 0, 0.9f }, { 1, 1, 3, 0.7f }, { 2, 1, 7, 0.75f }, { 3, 1, 0, 0.85f }, { 4, 1, 3, 0.7f }, { 5, 1, 7, 0.75f },
                                         { 6, 1, -2, 0.85f }, { 7, 1, 3, 0.7f }, { 8, 1, 7, 0.75f }, { 9, 1, -2, 0.85f }, { 10, 1, 2, 0.7f }, { 11, 1, 7, 0.75f } }, bar / 12.0), bar },
            { "Bell Chord", chord ({ 60, 67, 72, 75 }, 0, 12, 0.8f), bar },
        };
    }

    std::vector<Pattern> heldSounds()
    {
        return {
            { "Held Note", { { 48, 0.0, 2000.0, 0.85f } }, 0.0 },
            { "Two Bars Held", line (48, { { 0, 31, 0, 0.85f } }), 2 * bar },
            { "Four Bars Held", line (48, { { 0, 63, 0, 0.85f } }), 4 * bar },
            { "Low Held", line (36, { { 0, 31, 0, 0.85f } }), 2 * bar },
            { "High Held", line (60, { { 0, 31, 0, 0.85f } }), 2 * bar },
            { "Chord Held", chord ({ 48, 55, 63 }, 0, 31, 0.8f), 2 * bar },
            { "Each Bar", line (48, { { 0, 15, 0, 0.85f }, { 16, 15, 0, 0.85f } }), 2 * bar },
            { "Two Notes", line (48, { { 0, 15, 0, 0.85f }, { 16, 15, 3, 0.85f } }), 2 * bar },
        };
    }

    std::vector<Pattern> hitSounds()
    {
        return {
            { "Two Hits", { { 60, 0.0, 120.0, 0.9f }, { 60, 380.0, 120.0, 0.7f } }, 0.0 },
            { "Quarters", line (60, { { 0, 1, 0, 0.95f }, { 4, 1, 0, 0.85f }, { 8, 1, 0, 0.9f }, { 12, 1, 0, 0.85f } }), bar },
            { "Offbeats", line (60, { { 2, 1, 0, 0.9f }, { 6, 1, 0, 0.85f }, { 10, 1, 0, 0.9f }, { 14, 1, 0, 0.85f } }), bar },
            { "16ths", line (60, { { 0, 1, 0, 1.0f }, { 1, 1, 0, 0.5f }, { 2, 1, 0, 0.7f }, { 3, 1, 0, 0.5f }, { 4, 1, 0, 0.9f }, { 5, 1, 0, 0.5f }, { 6, 1, 0, 0.7f }, { 7, 1, 0, 0.5f },
                                   { 8, 1, 0, 0.9f }, { 9, 1, 0, 0.5f }, { 10, 1, 0, 0.7f }, { 11, 1, 0, 0.5f }, { 12, 1, 0, 0.9f }, { 13, 1, 0, 0.5f }, { 14, 1, 0, 0.7f }, { 15, 1, 0, 0.5f } }), bar },
            { "One Hit", line (60, { { 0, 1, 0, 0.95f } }), 2 * bar },
            { "Pitched", line (48, { { 0, 1, 0, 0.9f }, { 3, 1, 7, 0.8f }, { 6, 1, 12, 0.85f }, { 8, 1, 0, 0.9f }, { 11, 1, 19, 0.8f }, { 14, 1, 12, 0.85f } }), bar },
            { "Syncopated", line (60, { { 0, 1, 0, 0.95f }, { 3, 1, 0, 0.8f }, { 6, 1, 0, 0.85f }, { 10, 1, 0, 0.9f }, { 12, 1, 0, 0.8f } }), bar },
            { "Roll", line (60, { { 0, 1, 0, 0.4f }, { 1, 1, 0, 0.45f }, { 2, 1, 0, 0.5f }, { 3, 1, 0, 0.55f }, { 4, 1, 0, 0.6f }, { 5, 1, 0, 0.65f }, { 6, 1, 0, 0.7f }, { 7, 1, 0, 0.75f },
                                  { 8, 1, 0, 0.8f }, { 9, 1, 0, 0.85f }, { 10, 1, 0, 0.9f }, { 11, 1, 0, 0.95f }, { 12, 1, 0, 1.0f } }, sixteenth / 2.0), bar },
        };
    }

    std::vector<Pattern> stringSounds()
    {
        return {
            { "Low E Held", { { 40, 0.0, 1500.0, 0.85f } }, 0.0 },
            { "Power Chord", chord ({ 40, 47, 52 }, 0, 31, 0.85f), 2 * bar },
            { "Chugs", line (40, { { 0, 1, 0, 0.95f }, { 2, 1, 0, 0.8f }, { 4, 1, 0, 0.9f }, { 6, 1, 0, 0.8f }, { 8, 1, 0, 0.9f }, { 10, 1, 0, 0.8f }, { 12, 1, 0, 0.9f }, { 14, 1, 0, 0.8f } }), bar },
            { "Gallop", line (40, { { 0, 2, 0, 0.95f }, { 2, 1, 0, 0.75f }, { 3, 1, 0, 0.8f }, { 4, 2, 0, 0.9f }, { 6, 1, 0, 0.75f }, { 7, 1, 0, 0.8f },
                                    { 8, 2, 0, 0.9f }, { 10, 1, 0, 0.75f }, { 11, 1, 0, 0.8f }, { 12, 2, 3, 0.9f }, { 14, 1, 3, 0.75f }, { 15, 1, 3, 0.8f } }), bar },
            { "Two Notes", line (40, { { 0, 15, 0, 0.85f }, { 16, 15, 3, 0.85f } }), 2 * bar },
            { "High Drone", line (52, { { 0, 31, 0, 0.85f } }), 2 * bar },
            { "Slow Swell", line (40, { { 0, 63, 0, 0.6f } }), 4 * bar },
            { "Riff", line (40, { { 0, 2, 0, 0.95f }, { 3, 1, 0, 0.8f }, { 4, 2, 3, 0.9f }, { 7, 1, 0, 0.8f }, { 8, 2, 5, 0.9f }, { 11, 1, 3, 0.8f }, { 12, 4, 0, 0.9f } }), bar },
        };
    }

    std::vector<Pattern> kits()
    {
        constexpr int kick = 36, rim = 37, snare = 38, clap = 39, closedHat = 42, openHat = 46, lowTom = 41, midTom = 45, highTom = 48, crash = 49, ride = 51;
        return {
            { "Kick Hat Snare", { { 36, 0.0, 60.0, 0.95f }, { 42, 125.0, 40.0, 0.6f }, { 38, 250.0, 60.0, 0.9f }, { 42, 375.0, 40.0, 0.6f }, { 36, 500.0, 60.0, 0.9f }, { 39, 750.0, 60.0, 0.85f } }, 0.0 },
            { "Four on the Floor", join ({ hits (kick, { 0, 4, 8, 12 }, 0.95f), hits (openHat, { 2, 6, 10, 14 }, 0.7f), hits (clap, { 4, 12 }, 0.85f) }), bar },
            { "Techno 16ths", join ({ hits (kick, { 0, 4, 8, 12 }, 0.95f), hits (closedHat, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }, 0.65f, 0.4f),
                                      hits (clap, { 4, 12 }, 0.85f), hits (openHat, { 14 }, 0.6f) }), bar },
            { "Machine March", join ({ hits (kick, { 0, 4, 8, 12 }, 1.0f), hits (snare, { 4, 12 }, 0.95f), hits (closedHat, { 0, 2, 4, 6, 8, 10, 12, 14 }, 0.6f) }), bar },
            { "Industrial", join ({ hits (kick, { 0, 3, 8, 11 }, 1.0f), hits (snare, { 4, 12 }, 0.95f), hits (rim, { 7, 14, 15 }, 0.7f), hits (lowTom, { 10 }, 0.8f) }), bar },
            { "Broken", join ({ hits (kick, { 0, 6, 10 }, 0.95f), hits (snare, { 4, 12 }, 0.9f), hits (closedHat, { 2, 6, 10, 14 }, 0.6f), hits (closedHat, { 15 }, 0.45f) }), bar },
            { "Half-Time", join ({ hits (kick, { 0, 10 }, 0.95f), hits (snare, { 8 }, 0.95f), hits (closedHat, { 0, 2, 4, 6, 8, 10, 12, 14 }, 0.55f) }), bar },
            { "Rolling Kick", join ({ hits (kick, { 0, 4, 8, 12, 15 }, 0.95f), hits (openHat, { 2, 6, 10, 14 }, 0.65f), hits (clap, { 4, 12 }, 0.8f) }), bar },
            { "Electro", join ({ hits (kick, { 0, 7, 10 }, 0.95f), hits (clap, { 4, 12 }, 0.9f), hits (closedHat, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }, 0.55f, 0.35f) }), bar },
            { "With a Fill", join ({ hits (kick, { 0, 4, 8, 12, 16, 20, 24 }, 0.95f), hits (clap, { 4, 12, 20 }, 0.85f), hits (closedHat, { 2, 6, 10, 14, 18, 22 }, 0.6f),
                                     hits (highTom, { 26, 27 }, 0.75f), hits (midTom, { 28, 29 }, 0.8f), hits (lowTom, { 30, 31 }, 0.9f), hits (crash, { 0 }, 0.7f) }), 2 * bar },
            { "Ride Groove", join ({ hits (kick, { 0, 4, 8, 12 }, 0.95f), hits (ride, { 0, 2, 4, 6, 8, 10, 12, 14 }, 0.6f), hits (clap, { 4, 12 }, 0.8f) }), bar },
            { "Kick and Clap", join ({ hits (kick, { 0, 4, 8, 12 }, 0.95f), hits (clap, { 4, 12 }, 0.9f) }), bar },

            // Grooves that play more of a kit, with ghost notes and accents: a rumble, a march, industrial and breaks.
            { "Warehouse Roller", join ({ hits (kick, { 0, 4, 8, 12, 16, 20, 24, 28 }, 0.95f), hits (kick, { 2, 3, 6, 7, 10, 11, 14, 18, 19, 22, 23, 26, 27, 30 }, 0.32f),
                                          hits (openHat, { 2, 6, 10, 14, 18, 22, 26, 30 }, 0.7f), hits (clap, { 4, 12, 20, 28 }, 0.85f),
                                          hits (closedHat, { 0, 4, 8, 12, 16, 20, 24, 28 }, 0.5f), hits (rim, { 7, 14, 23 }, 0.6f),
                                          hits (ride, { 16, 20, 24, 28 }, 0.75f), hits (ride, { 18, 22, 26, 30 }, 0.55f), hits (clap, { 31 }, 0.6f) }), 2 * bar },
            { "Body Beat", join ({ hits (kick, { 0, 8, 10, 16, 22, 24, 26 }, 0.95f), hits (snare, { 4, 12, 20, 28 }, 0.95f),
                                   hits (closedHat, { 0, 4, 8, 12, 16, 20, 24 }, 0.75f), hits (closedHat, { 2, 6, 10, 14, 18, 22, 26 }, 0.5f),
                                   hits (highTom, { 29 }, 0.8f), hits (midTom, { 30 }, 0.85f), hits (lowTom, { 31 }, 0.95f) }), 2 * bar },
            { "Hammer Works", join ({ hits (kick, { 0, 4, 8, 12, 16, 20, 24, 28 }, 1.0f), hits (lowTom, { 2, 6, 10, 13, 15, 18, 22, 26 }, 0.8f),
                                      hits (snare, { 4, 12, 20, 28 }, 0.95f), hits (snare, { 29, 30, 31 }, 0.45f, 0.6f), hits (52, { 0 }, 0.8f),
                                      hits (closedHat, { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30 }, 0.6f), hits (rim, { 3, 10, 19, 27 }, 0.6f) }), 2 * bar },
            { "Two-Step Break", join ({ hits (kick, { 0, 2, 10, 16, 26, 29 }, 0.95f), hits (snare, { 4, 12, 20, 28 }, 0.95f), hits (snare, { 7, 9, 23, 25, 30 }, 0.32f),
                                        hits (closedHat, { 0, 4, 8, 12, 16, 20, 24, 28 }, 0.75f), hits (closedHat, { 2, 6, 10, 14, 18, 22, 26 }, 0.55f),
                                        hits (openHat, { 30 }, 0.7f) }), 2 * bar },
            { "Hypnotic Threes", join ({ hits (kick, { 0, 4, 8, 12, 16, 20, 24, 28 }, 0.95f), hits (rim, { 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30 }, 0.6f),
                                         hits (closedHat, { 2, 6, 10, 14, 18, 22, 26, 30 }, 0.7f), hits (70, { 2, 7, 12, 17, 22, 27 }, 0.5f),
                                         hits (lowTom, { 26 }, 0.6f) }), 2 * bar },
            { "Peak Time", join ({ hits (kick, { 0, 4, 8, 12, 16, 20, 24, 28 }, 0.95f), hits (clap, { 4, 12, 20, 28 }, 0.9f), hits (openHat, { 2, 6, 10, 14, 18, 22, 26, 30 }, 0.75f),
                                   hits (closedHat, { 0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16, 17, 19, 20, 21, 23, 24, 25, 27 }, 0.6f, 0.4f),
                                   hits (ride, { 0, 4, 8, 12, 16, 20, 24 }, 0.7f), hits (crash, { 0 }, 0.75f),
                                   hits (snare, { 28, 29, 30, 31 }, 0.5f, 0.65f) }), 2 * bar },
            { "Machine Disco", join ({ hits (kick, { 0, 4, 8, 12 }, 0.95f), hits (snare, { 4, 12 }, 0.9f), hits (clap, { 4, 12 }, 0.8f), hits (openHat, { 2, 6, 10, 14 }, 0.7f),
                                       hits (closedHat, { 0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15 }, 0.6f) }), bar },
            { "Ghost Kicks", join ({ hits (kick, { 0, 4, 8, 12 }, 0.95f), hits (kick, { 3, 11 }, 0.32f), hits (clap, { 4, 12 }, 0.9f), hits (snare, { 4, 12 }, 0.55f),
                                     hits (closedHat, { 0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15 }, 0.5f, 0.45f), hits (closedHat, { 2, 6, 10, 14 }, 0.8f) }), bar },
        };
    }
}

const std::vector<Pattern>& patternsFor (Kind kind)
{
    static const std::vector<Pattern> bass = basses(), chordList = chordSounds(), stabList = stabs(), lead = leads(),
                                      held = heldSounds(), hit = hitSounds(), strings = stringSounds(), kit = kits(),
                                      other { { "Middle C", { { 60, 0.0, 700.0, 0.8f } }, 0.0 } };

    switch (kind)
    {
        case Kind::bass:    return bass;
        case Kind::chord:   return chordList;
        case Kind::stab:    return stabList;
        case Kind::lead:    return lead;
        case Kind::held:    return held;
        case Kind::hit:     return hit;
        case Kind::strings: return strings;
        case Kind::kit:     return kit;
        case Kind::other:   break;
    }

    return other;
}
}
