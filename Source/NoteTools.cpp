#include "NoteTools.h"

namespace NoteTools
{
namespace
{
    struct Mode
    {
        const char* name;
        std::vector<int> steps;
    };

    const std::vector<Mode>& allModes()
    {
        static const std::vector<Mode> m {
            { "major",            { 0, 2, 4, 5, 7, 9, 11 } },
            { "minor",            { 0, 2, 3, 5, 7, 8, 10 } },
            { "harmonic minor",   { 0, 2, 3, 5, 7, 8, 11 } },
            { "melodic minor",    { 0, 2, 3, 5, 7, 9, 11 } },
            { "dorian",           { 0, 2, 3, 5, 7, 9, 10 } },
            { "phrygian",         { 0, 1, 3, 5, 7, 8, 10 } },
            { "lydian",           { 0, 2, 4, 6, 7, 9, 11 } },
            { "mixolydian",       { 0, 2, 4, 5, 7, 9, 10 } },
            { "locrian",          { 0, 1, 3, 5, 6, 8, 10 } },
            { "major pentatonic", { 0, 2, 4, 7, 9 } },
            { "minor pentatonic", { 0, 3, 5, 7, 10 } },
            { "blues",            { 0, 3, 5, 6, 7, 10 } },
        };
        return m;
    }

    const char* const pitchNames[] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };

    int classOf (int pitch) noexcept   { return ((pitch % 12) + 12) % 12; }

    /** A pitch class written at the start of text ("A", "Bb", "F#"), and how many characters it took. */
    std::optional<std::pair<int, int>> readPitchClass (const juce::String& text)
    {
        static const int letters[] = { 9, 11, 0, 2, 4, 5, 7 };   // A to G
        const auto first = juce::CharacterFunctions::toUpperCase (text[0]);

        if (first < 'A' || first > 'G')
            return std::nullopt;

        auto pc = letters[first - 'A'];
        auto used = 1;

        if (text[1] == '#' || text[1] == (juce::juce_wchar) 0x266F)       { ++pc; ++used; }
        else if (text[1] == 'b' || text[1] == (juce::juce_wchar) 0x266D)  { --pc; ++used; }

        return std::make_pair (classOf (pc), used);
    }

    juce::int64 spread (juce::Random& r, juce::int64 amount)
    {
        return (juce::int64) std::llround ((r.nextDouble() * 2.0 - 1.0) * (double) amount);
    }
}

//==============================================================================
bool Scale::contains (int pitch) const noexcept
{
    return std::find (steps.begin(), steps.end(), classOf (pitch - root)) != steps.end();
}

int Scale::snap (int pitch) const noexcept
{
    for (int d = 0; d < 12; ++d)
    {
        if (pitch - d >= 0 && contains (pitch - d))
            return pitch - d;

        if (pitch + d <= 127 && contains (pitch + d))
            return pitch + d;
    }

    return pitch;
}

int Scale::step (int pitch, int degrees) const noexcept
{
    auto p = snap (pitch);

    for (int i = 0; i < std::abs (degrees); ++i)
    {
        const auto direction = degrees > 0 ? 1 : -1;
        auto next = p + direction;

        while (next >= 0 && next <= 127 && ! contains (next))
            next += direction;

        if (next < 0 || next > 127)
            break;

        p = next;
    }

    return p;
}

int Scale::degree (int number, int octave) const noexcept
{
    const auto size = juce::jmax (1, (int) steps.size());
    const auto n = juce::jmax (0, number - 1);
    return 12 * (octave + 1) + root + steps[(size_t) (n % size)] + 12 * (n / size);
}

juce::String Scale::name() const
{
    return juce::String (pitchNames[classOf (root)]) + " " + mode;
}

juce::StringArray modes()
{
    juce::StringArray names;

    for (auto& m : allModes())
        names.add (m.name);

    return names;
}

std::optional<Scale> parseKey (const juce::String& text)
{
    const auto t = text.trim();
    const auto pc = readPitchClass (t);

    if (! pc.has_value())
        return std::nullopt;

    auto rest = t.substring (pc->second).trim().toLowerCase();

    if (rest.isEmpty() || rest == "maj" || rest == "ionian")
        rest = "major";
    else if (rest == "m" || rest == "min" || rest == "aeolian")
        rest = "minor";

    for (auto& m : allModes())
        if (rest == m.name)
            return Scale { pc->first, m.name, m.steps };

    return std::nullopt;
}

Scale guessKey (const std::vector<ClipNote>& notes)
{
    std::array<double, 12> weight {};
    const ClipNote* first = nullptr;

    for (auto& n : notes)
    {
        weight[(size_t) classOf (n.pitch)] += (double) juce::jmax ((juce::int64) 1, n.length);

        if (first == nullptr || n.start < first->start)
            first = &n;
    }

    Scale best;
    double bestScore = -1.0;
    auto bestOnFirst = false;

    for (int root = 0; root < 12; ++root)
        for (int m = 0; m < 2; ++m)
        {
            const Scale s { root, allModes()[(size_t) m].name, allModes()[(size_t) m].steps };
            double score = 0.0;

            for (int pc = 0; pc < 12; ++pc)
                if (s.contains (pc))
                    score += weight[(size_t) pc];

            const auto onFirst = first != nullptr && classOf (first->pitch) == root;

            if (score > bestScore || (juce::exactlyEqual (score, bestScore) && onFirst && ! bestOnFirst))
            {
                best = s;
                bestScore = score;
                bestOnFirst = onFirst;
            }
        }

    return best;
}

//==============================================================================
std::vector<Chord> parseProgression (const juce::String& text, const Scale& scale, int octave, juce::String& error)
{
    error.clear();

    if (scale.steps.size() != 7)
    {
        error = "Roman numerals need a scale of seven notes; " + scale.name() + " has " + juce::String ((int) scale.steps.size()) + ".";
        return {};
    }

    auto spaced = text;

    for (auto separator : { "-", ",", "|", "\xe2\x80\x93", "\xe2\x80\x94" })
        spaced = spaced.replace (juce::String::fromUTF8 (separator), " ");

    std::vector<Chord> chords;

    for (auto token : juce::StringArray::fromTokens (spaced, " \t\n", {}))
    {
        if (token.isEmpty())
            continue;

        int i = 0, accidental = 0;

        if ((token[0] == 'b' || token[0] == (juce::juce_wchar) 0x266D) && juce::String ("IViv").containsChar (token[1]))       { accidental = -1; ++i; }
        else if ((token[0] == '#' || token[0] == (juce::juce_wchar) 0x266F) && juce::String ("IViv").containsChar (token[1]))  { accidental = 1; ++i; }

        juce::String numeral;

        while (i < token.length() && juce::String ("IViv").containsChar (token[i]))
            numeral << juce::String::charToString (token[i++]);

        const auto value = juce::StringArray { "I", "II", "III", "IV", "V", "VI", "VII" }.indexOf (numeral.toUpperCase()) + 1;

        if (value <= 0 || (numeral != numeral.toUpperCase() && numeral != numeral.toLowerCase()))
        {
            error = "\"" + token + "\" is not a Roman numeral from I to VII.";
            return {};
        }

        auto rest = token.substring (i).trimCharactersAtStart ("/").toLowerCase();
        std::vector<int> triad = numeral == numeral.toUpperCase() ? std::vector<int> { 0, 4, 7 } : std::vector<int> { 0, 3, 7 };
        int seventh = -1;
        const auto take = [&rest] (const juce::String& part)
        {
            if (! rest.contains (part))
                return false;

            rest = rest.replaceFirstOccurrenceOf (part, {});
            return true;
        };
        const auto halfDiminished = juce::String::fromUTF8 ("\xc3\xb8"), diminished = juce::String::fromUTF8 ("\xc2\xb0");

        // The symbols one at a time, then a 7: a half-diminished sign brings its seventh; with the diminished sign or dim, a 7 is diminished.
        auto halfDim = false;

        if (take ("maj7"))
            seventh = 11;

        if (take ("m7b5") || take (halfDiminished))
        {
            triad = { 0, 3, 6 };
            halfDim = true;
        }
        else if (take (diminished) || take ("dim") || rest == "o" || rest == "o7")
        {
            triad = { 0, 3, 6 };
            rest = rest.replaceFirstOccurrenceOf ("o", {});
        }
        else if (take ("+") || take ("aug"))   triad = { 0, 4, 8 };
        else if (take ("sus2"))                triad = { 0, 2, 7 };
        else if (take ("sus4") || take ("sus")) triad = { 0, 5, 7 };

        if (take ("7") || halfDim)
            seventh = seventh >= 0 ? seventh : halfDim ? 10 : triad == std::vector<int> { 0, 3, 6 } ? 9 : 10;

        if (rest.trim().isNotEmpty())
        {
            error = "\"" + token + "\": \"" + rest.trim() + "\" can't be read "
                    + juce::String::fromUTF8 ("(use 7, maj7, dim or \xc2\xb0, \xc3\xb8 or m7b5, aug or +, sus2, sus4).");
            return {};
        }

        const auto root = scale.degree (value, octave) + accidental;
        Chord c;
        c.symbol = token;

        for (auto interval : triad)
            c.pitches.push_back (juce::jlimit (0, 127, root + interval));

        if (seventh >= 0)
            c.pitches.push_back (juce::jlimit (0, 127, root + seventh));

        chords.push_back (std::move (c));
    }

    if (chords.empty())
        error = "Write the chords as Roman numerals, such as i VI III VII.";

    return chords;
}

std::vector<ClipNote> chordNotes (const std::vector<Chord>& chords, juce::int64 start, juce::int64 each, float velocity)
{
    std::vector<ClipNote> notes;
    each = juce::jmax ((juce::int64) 1, each);

    for (size_t i = 0; i < chords.size(); ++i)
        for (auto p : chords[i].pitches)
            notes.push_back ({ p, start + (juce::int64) i * each, each, velocity });

    return notes;
}

//==============================================================================
juce::StringArray arpOrderNames()
{
    return { "Up", "Down", "Up and Down", "Random" };
}

std::vector<ClipNote> arpeggiate (const std::vector<ClipNote>& notes, const Arp& arp)
{
    if (arp.step <= 0 || notes.empty())
        return notes;

    auto sorted = notes;
    std::stable_sort (sorted.begin(), sorted.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start < b.start; });

    juce::Random random ((juce::int64) arp.seed);
    std::vector<ClipNote> out;
    int previous = -1;

    for (size_t g = 0; g < sorted.size();)
    {
        const auto groupStart = sorted[g].start;
        auto groupEnd = groupStart;
        std::vector<int> pitches;
        float velocity = 0.0f;
        size_t count = 0;

        for (; g < sorted.size() && sorted[g].start - groupStart <= arp.step / 2; ++g, ++count)
        {
            groupEnd = juce::jmax (groupEnd, sorted[g].start + juce::jmax ((juce::int64) 1, sorted[g].length));
            pitches.push_back (sorted[g].pitch);
            velocity += sorted[g].velocity;
        }

        velocity /= (float) count;
        std::sort (pitches.begin(), pitches.end());
        pitches.erase (std::unique (pitches.begin(), pitches.end()), pitches.end());

        std::vector<int> pool;

        for (int o = 0; o < juce::jlimit (1, 4, arp.octaves); ++o)
            for (auto p : pitches)
                if (p + 12 * o <= 127)
                    pool.push_back (p + 12 * o);

        std::vector<int> order = pool;

        if (arp.order == ArpOrder::down)
            std::reverse (order.begin(), order.end());
        else if (arp.order == ArpOrder::upDown && pool.size() > 2)
            order.insert (order.end(), pool.rbegin() + 1, pool.rend() - 1);

        int k = 0;

        // Every step that fits, the last one only with half a step left in the chord.
        for (auto t = groupStart; t == groupStart || t + arp.step / 2 <= groupEnd; t += arp.step, ++k)
        {
            int pitch = 0;

            if (arp.order == ArpOrder::random)
            {
                const auto n = (int) pool.size();
                auto pick = random.nextInt (n);

                if (n > 1 && pick == previous)
                    pick = (pick + 1 + random.nextInt (n - 1)) % n;

                previous = pick;
                pitch = pool[(size_t) pick];
            }
            else
            {
                pitch = order[(size_t) k % order.size()];
            }

            const auto length = juce::jlimit ((juce::int64) 1, groupEnd - t, (juce::int64) std::llround ((double) arp.step * juce::jlimit (0.05, 1.0, arp.gate)));
            out.push_back ({ pitch, t, length, velocity });
        }
    }

    return out;
}

//==============================================================================
void humanize (std::vector<ClipNote>& notes, const std::set<int>& which, const Humanize& h, juce::int64 length)
{
    juce::Random random ((juce::int64) h.seed);

    for (int i = 0; i < (int) notes.size(); ++i)
    {
        const auto dt = spread (random, h.timing);
        const auto dv = (random.nextFloat() * 2.0f - 1.0f) * h.velocity;

        if (! which.empty() && which.count (i) == 0)
            continue;

        auto& n = notes[(size_t) i];
        n.start = juce::jlimit ((juce::int64) 0, juce::jmax ((juce::int64) 0, length - 1), n.start + dt);
        n.length = juce::jlimit ((juce::int64) 1, juce::jmax ((juce::int64) 1, length - n.start), n.length);
        n.velocity = juce::jlimit (1.0f / 127.0f, 1.0f, n.velocity * (1.0f + dv));
    }
}

void fitToScale (std::vector<ClipNote>& notes, const std::set<int>& which, const Scale& scale)
{
    for (int i = 0; i < (int) notes.size(); ++i)
        if (which.empty() || which.count (i) > 0)
            notes[(size_t) i].pitch = scale.snap (notes[(size_t) i].pitch);
}

std::vector<ClipNote> vary (const std::vector<ClipNote>& notes, const Vary& v, juce::int64 length)
{
    juce::Random random ((juce::int64) v.seed);
    std::vector<ClipNote> kept;

    for (auto n : notes)
    {
        const auto dv = (random.nextFloat() * 2.0f - 1.0f) * v.velocity;
        const auto moves = random.nextFloat() < v.pitch;
        const auto up = random.nextBool();
        const auto dropped = v.density < 0.0f && random.nextFloat() < -v.density;

        if (dropped)
            continue;

        n.velocity = juce::jlimit (1.0f / 127.0f, 1.0f, n.velocity * (1.0f + dv));

        if (moves)
            n.pitch = v.scale.step (n.pitch, up ? 1 : -1);

        kept.push_back (n);
    }

    if (kept.empty() && ! notes.empty())
        kept.push_back (notes.front());

    if (v.density > 0.0f && v.step > 0 && ! kept.empty())
    {
        auto byStart = kept;
        std::stable_sort (byStart.begin(), byStart.end(), [] (const ClipNote& a, const ClipNote& b) { return a.start < b.start; });
        std::vector<ClipNote> added;

        for (juce::int64 t = 0; t < length; t += v.step)
        {
            const auto taken = std::any_of (byStart.begin(), byStart.end(), [&] (const ClipNote& n) { return n.start >= t && n.start < t + v.step; });
            const auto draw = random.nextFloat();

            if (taken || draw >= v.density)
                continue;

            const ClipNote* model = &byStart.front();

            for (auto& n : byStart)
                if (n.start < t)
                    model = &n;

            added.push_back ({ model->pitch, t, juce::jlimit ((juce::int64) 1, juce::jmax ((juce::int64) 1, length - t), juce::jmin (v.step, model->length)),
                               juce::jmax (1.0f / 127.0f, model->velocity * 0.6f), model->probability });
        }

        kept.insert (kept.end(), added.begin(), added.end());
    }

    return kept;
}

std::vector<GrooveTemplate> groovePresets()
{
    auto makeSwing = [] (const char* name, double swingPct, float offVel = 0.94f)
    {
        GrooveTemplate g;
        g.name = name;
        g.stepsPerBeat = 4;
        const auto shift = (swingPct - 50.0) / 100.0 * 0.5; // shift on odd 16ths in beats
        g.beatOffsets.resize (16, 0.0);
        g.velocityScales.resize (16, 1.0f);
        for (int i = 0; i < 16; ++i)
            if (i % 2 == 1)
            {
                g.beatOffsets[(size_t) i] = shift;
                g.velocityScales[(size_t) i] = offVel;
            }
        return g;
    };

    std::vector<GrooveTemplate> list;
    list.push_back (makeSwing ("Straight", 50.0, 1.0f));
    list.push_back (makeSwing ("MPC 54%", 54.0, 0.96f));
    list.push_back (makeSwing ("MPC 58%", 58.0, 0.93f));
    list.push_back (makeSwing ("MPC 62%", 62.0, 0.90f));
    list.push_back (makeSwing ("MPC 66%", 66.0, 0.88f));

    GrooveTemplate shuffle;
    shuffle.name = "16th Shuffle";
    shuffle.stepsPerBeat = 4;
    shuffle.beatOffsets    = { 0.0, 0.06, 0.0, 0.075, 0.0, 0.06, 0.0, 0.08, 0.0, 0.06, 0.0, 0.075, 0.0, 0.065, 0.0, 0.085 };
    shuffle.velocityScales = { 1.05f, 0.85f, 0.95f, 0.88f, 1.05f, 0.85f, 0.95f, 0.88f, 1.05f, 0.85f, 0.95f, 0.88f, 1.05f, 0.85f, 0.95f, 0.90f };
    list.push_back (std::move (shuffle));

    GrooveTemplate push;
    push.name = "Push Forward";
    push.stepsPerBeat = 4;
    push.beatOffsets.assign (16, -0.025);
    push.velocityScales.assign (16, 1.02f);
    list.push_back (std::move (push));

    GrooveTemplate laid;
    laid.name = "Laid-Back";
    laid.stepsPerBeat = 4;
    laid.beatOffsets    = { 0.0, 0.03, 0.02, 0.035, 0.015, 0.03, 0.02, 0.035, 0.0, 0.03, 0.02, 0.035, 0.015, 0.03, 0.02, 0.04 };
    laid.velocityScales = { 1.0f, 0.92f, 0.96f, 0.90f, 1.02f, 0.92f, 0.96f, 0.90f, 1.0f, 0.92f, 0.96f, 0.90f, 1.02f, 0.92f, 0.96f, 0.90f };
    list.push_back (std::move (laid));

    return list;
}

GrooveTemplate grooveFromOffsets (const juce::String& name, const std::vector<double>& offsets, int stepsPerBeat)
{
    GrooveTemplate g;
    g.name = name.isNotEmpty() ? name : juce::String ("Extracted Groove");
    g.stepsPerBeat = juce::jmax (1, stepsPerBeat);
    g.beatOffsets.assign (16, 0.0);
    g.velocityScales.assign (16, 1.0f);
    for (size_t i = 0; i < juce::jmin (offsets.size(), g.beatOffsets.size()); ++i)
        g.beatOffsets[i] = juce::jlimit (-0.2, 0.2, offsets[i]);
    return g;
}

void applyGroove (std::vector<ClipNote>& notes, const std::set<int>& which, const GrooveTemplate& groove,
                  double beatFrames, float strength, juce::int64 length)
{
    if (notes.empty() || groove.beatOffsets.empty() || beatFrames <= 1.0 || strength <= 0.0f)
        return;

    const auto s = juce::jlimit (0.0f, 1.0f, strength);
    const auto stepFrames = beatFrames / (double) juce::jmax (1, groove.stepsPerBeat);
    const auto nSteps = (int) groove.beatOffsets.size();

    for (size_t i = 0; i < notes.size(); ++i)
    {
        if (! which.empty() && which.count ((int) i) == 0)
            continue;

        auto& n = notes[i];
        const auto stepIndex = ((int) std::llround ((double) n.start / stepFrames) % nSteps + nSteps) % nSteps;
        const auto offsetFrames = (juce::int64) std::llround (groove.beatOffsets[(size_t) stepIndex] * beatFrames * (double) s);
        n.start = juce::jlimit ((juce::int64) 0, juce::jmax ((juce::int64) 0, length - 1), n.start + offsetFrames);

        if ((int) groove.velocityScales.size() > stepIndex)
        {
            const auto scale = 1.0f + s * (groove.velocityScales[(size_t) stepIndex] - 1.0f);
            n.velocity = juce::jlimit (1.0f / 127.0f, 1.0f, n.velocity * scale);
        }
    }
}
}
