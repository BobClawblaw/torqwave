#include "Notation.h"
#include <algorithm>
#include <cmath>
#include <map>

namespace Notation
{
namespace
{
    // Diatonic offset within octave [0..6] (C=0, D=1, E=2, F=3, G=4, A=5, B=6)
    constexpr int sharpDiatonic[12]   = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
    constexpr int sharpAccidental[12] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };
    constexpr int flatDiatonic[12]    = { 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 };
    constexpr int flatAccidental[12]  = { 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0 };
    constexpr char stepLetters[7]     = { 'C', 'D', 'E', 'F', 'G', 'A', 'B' };
    constexpr int naturalSemis[7]     = { 0, 2, 4, 5, 7, 9, 11 };

    juce::String xmlEscape (const juce::String& s)
    {
        return s.replace ("&", "&amp;").replace ("<", "&lt;").replace (">", "&gt;").replace ("\"", "&quot;");
    }

    const char* durationTypeName (DurationKind k) noexcept
    {
        switch (k)
        {
            case DurationKind::whole:         return "whole";
            case DurationKind::dottedHalf:
            case DurationKind::half:          return "half";
            case DurationKind::dottedQuarter:
            case DurationKind::quarter:       return "quarter";
            case DurationKind::eighth:        return "eighth";
            case DurationKind::sixteenth:     return "16th";
        }
        return "quarter";
    }

    bool durationIsDotted (DurationKind k) noexcept
    {
        return k == DurationKind::dottedHalf || k == DurationKind::dottedQuarter;
    }

    int durationDivisions (DurationKind k) noexcept
    {
        // divisions = 4 per quarter note (16th-note resolution)
        switch (k)
        {
            case DurationKind::whole:         return 16;
            case DurationKind::dottedHalf:    return 12;
            case DurationKind::half:          return 8;
            case DurationKind::dottedQuarter: return 6;
            case DurationKind::quarter:       return 4;
            case DurationKind::eighth:        return 2;
            case DurationKind::sixteenth:     return 1;
        }
        return 4;
    }
}

int keyFifths (const NoteTools::Scale& key) noexcept
{
    const bool isMinor = key.mode.containsIgnoreCase ("minor")
                      || key.mode.containsIgnoreCase ("dorian")
                      || key.mode.containsIgnoreCase ("phrygian");
    // Relative major pitch class
    const int majorRoot = ((key.root + (isMinor ? 3 : 0)) % 12 + 12) % 12;
    // Pitch class -> circle of fifths (-5..+6)
    static constexpr int fifthsMap[12] = { 0, -5, 2, -3, 4, -1, 6, 1, -4, 3, -2, 5 };
    return fifthsMap[majorRoot];
}

bool prefersFlats (const NoteTools::Scale& key) noexcept
{
    return keyFifths (key) < 0;
}

PitchSpelling spell (int midiPitch, bool useFlats) noexcept
{
    const int p = juce::jlimit (0, 127, midiPitch);
    const int pc = p % 12;
    const int oct = p / 12 - 1; // MIDI 60 = C4 -> oct = 4
    const int dInOct = useFlats ? flatDiatonic[pc] : sharpDiatonic[pc];
    const int acc = useFlats ? flatAccidental[pc] : sharpAccidental[pc];

    PitchSpelling sp;
    sp.octave = oct;
    sp.step = stepLetters[dInOct];
    sp.accidental = acc;
    sp.diatonic = oct * 7 + dInOct; // C4 -> 4*7 + 0 = 28
    return sp;
}

int diatonicToMidi (int diatonic, const NoteTools::Scale& key) noexcept
{
    const int clamped = juce::jlimit (7, 63, diatonic); // C1 .. C9
    const int oct = clamped / 7;
    const int stepIdx = clamped % 7;
    int midi = (oct + 1) * 12 + naturalSemis[stepIdx];

    // Apply key signature accidental for this diatonic letter
    const int fifths = keyFifths (key);
    static constexpr int sharpOrder[7] = { 3, 0, 4, 1, 5, 2, 6 }; // F C G D A E B
    static constexpr int flatOrder[7]  = { 6, 2, 5, 1, 4, 0, 3 }; // B E A D G C F

    if (fifths > 0)
    {
        for (int i = 0; i < juce::jmin (7, fifths); ++i)
            if (sharpOrder[i] == stepIdx)
                ++midi;
    }
    else if (fifths < 0)
    {
        for (int i = 0; i < juce::jmin (7, -fifths); ++i)
            if (flatOrder[i] == stepIdx)
                --midi;
    }

    return juce::jlimit (0, 127, midi);
}

DurationKind classifyDuration (double beats) noexcept
{
    if (beats >= 3.5)   return DurationKind::whole;
    if (beats >= 2.5)   return DurationKind::dottedHalf;
    if (beats >= 1.75)  return DurationKind::half;
    if (beats >= 1.25)  return DurationKind::dottedQuarter;
    if (beats >= 0.75)  return DurationKind::quarter;
    if (beats >= 0.375) return DurationKind::eighth;
    return DurationKind::sixteenth;
}

GrandStaff GrandStaff::fit (juce::Rectangle<float> area) noexcept
{
    GrandStaff gs;
    // 4 spaces in treble + 2 spaces gap around middle C + 4 spaces in bass = 10 spaces total + margins (~18 spaces)
    gs.space = juce::jlimit (6.0f, 14.0f, area.getHeight() / 17.5f);
    gs.middleCY = area.getCentreY();
    gs.trebleBottomY = gs.middleCY - gs.space;  // E4 is 2 diatonic steps above C4 -> 1 full staff space
    gs.bassTopY = gs.middleCY + gs.space;       // A3 is 2 diatonic steps below C4 -> 1 full staff space
    return gs;
}

float GrandStaff::yForDiatonic (int diatonic) const noexcept
{
    // Middle C (28) sits cleanly between treble bottom line E4 (30) and bass top line A3 (26).
    // Each diatonic step is half a staff space (space * 0.5f).
    return middleCY - (float) (diatonic - 28) * (space * 0.5f);
}

int GrandStaff::diatonicAtY (float y) const noexcept
{
    const float halfSpace = juce::jmax (1.0f, space * 0.5f);
    return juce::jlimit (7, 63, 28 + (int) std::lround ((middleCY - y) / halfSpace));
}

juce::String toMusicXml (const NoteClip& clip, double sampleRate, double beatSeconds,
                         int beatsPerBar, const NoteTools::Scale& key, const juce::String& title)
{
    const double sr = juce::jmax (1.0, sampleRate);
    const double beatSec = juce::jmax (0.05, beatSeconds);
    const double framesPerSixteenth = (beatSec * sr) / 4.0;
    const int bpb = juce::jlimit (1, 16, beatsPerBar);
    const int divsPerMeasure = bpb * 4;
    const int fifths = keyFifths (key);
    const bool flats = fifths < 0;

    // Group active notes by quantized 16th-note onset
    struct QNote
    {
        int startDiv = 0;
        int durDiv = 4;
        DurationKind kind = DurationKind::quarter;
        PitchSpelling sp;
        int staff = 1;
        int pitch = 60;
    };

    std::vector<QNote> qnotes;
    int maxDiv = divsPerMeasure;

    for (const auto& n : clip.notes)
    {
        if (n.start < 0 || n.length <= 0)
            continue;

        QNote q;
        q.startDiv = juce::jmax (0, (int) std::llround ((double) n.start / framesPerSixteenth));
        const double beats = (double) n.length / (beatSec * sr);
        q.kind = classifyDuration (beats);
        q.durDiv = durationDivisions (q.kind);
        q.sp = spell (n.pitch, flats);
        q.staff = n.pitch >= 60 ? 1 : 2;
        q.pitch = n.pitch;
        maxDiv = juce::jmax (maxDiv, q.startDiv + q.durDiv);
        qnotes.push_back (q);
    }

    std::sort (qnotes.begin(), qnotes.end(), [] (const QNote& a, const QNote& b)
    {
        if (a.startDiv != b.startDiv) return a.startDiv < b.startDiv;
        return a.pitch < b.pitch;
    });

    const int numMeasures = juce::jmax (1, (maxDiv + divsPerMeasure - 1) / divsPerMeasure);
    const auto safeTitle = xmlEscape (title.isNotEmpty() ? title : (clip.name.isNotEmpty() ? clip.name : juce::String ("Torqwave Score")));

    juce::String xml;
    xml.preallocateBytes (4096);
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 3.1 Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">\n"
        << "<score-partwise version=\"3.1\">\n"
        << "  <work><work-title>" << safeTitle << "</work-title></work>\n"
        << "  <identification><encoding><software>Torqwave</software></encoding></identification>\n"
        << "  <part-list>\n"
        << "    <score-part id=\"P1\"><part-name>" << safeTitle << "</part-name></score-part>\n"
        << "  </part-list>\n"
        << "  <part id=\"P1\">\n";

    for (int m = 0; m < numMeasures; ++m)
    {
        const int mStart = m * divsPerMeasure;
        const int mEnd = mStart + divsPerMeasure;
        xml << "    <measure number=\"" << (m + 1) << "\">\n";

        if (m == 0)
        {
            xml << "      <attributes>\n"
                << "        <divisions>4</divisions>\n"
                << "        <key><fifths>" << fifths << "</fifths></key>\n"
                << "        <time><beats>" << bpb << "</beats><beat-type>4</beat-type></time>\n"
                << "        <staves>2</staves>\n"
                << "        <clef number=\"1\"><sign>G</sign><line>2</line></clef>\n"
                << "        <clef number=\"2\"><sign>F</sign><line>4</line></clef>\n"
                << "      </attributes>\n";
        }

        // Group notes falling inside this measure by onset
        std::map<int, std::vector<const QNote*>> byOnset;
        for (const auto& q : qnotes)
            if (q.startDiv >= mStart && q.startDiv < mEnd)
                byOnset[q.startDiv - mStart].push_back (&q);

        auto writeRest = [&] (int restDivs)
        {
            int rem = restDivs;
            static constexpr std::pair<int, const char*> restTable[] = {
                { 16, "whole" }, { 8, "half" }, { 4, "quarter" }, { 2, "eighth" }, { 1, "16th" }
            };
            while (rem > 0)
            {
                for (auto [d, name] : restTable)
                {
                    if (rem >= d)
                    {
                        xml << "      <note><rest/><duration>" << d << "</duration><type>" << name << "</type><staff>1</staff></note>\n";
                        rem -= d;
                        break;
                    }
                }
            }
        };

        int cursor = 0;
        for (const auto& [onset, group] : byOnset)
        {
            if (onset > cursor)
                writeRest (onset - cursor);

            int advance = 1;
            for (size_t idx = 0; idx < group.size(); ++idx)
            {
                const auto& q = *group[idx];
                const int dur = juce::jlimit (1, divsPerMeasure - onset, q.durDiv);
                if (idx == 0)
                    advance = dur;

                xml << "      <note>\n";
                if (idx > 0)
                    xml << "        <chord/>\n";
                xml << "        <pitch><step>" << juce::String::charToString ((juce::juce_wchar) q.sp.step) << "</step>";
                if (q.sp.accidental != 0)
                    xml << "<alter>" << q.sp.accidental << "</alter>";
                xml << "<octave>" << q.sp.octave << "</octave></pitch>\n"
                    << "        <duration>" << dur << "</duration>\n"
                    << "        <type>" << durationTypeName (q.kind) << "</type>\n";
                if (durationIsDotted (q.kind))
                    xml << "        <dot/>\n";
                xml << "        <staff>" << q.staff << "</staff>\n"
                    << "      </note>\n";
            }

            cursor = onset + advance;
        }

        if (cursor < divsPerMeasure)
            writeRest (divsPerMeasure - cursor);

        xml << "    </measure>\n";
    }

    xml << "  </part>\n"
        << "</score-partwise>\n";
    return xml;
}

juce::String exportMusicXml (const NoteClip& clip, double sampleRate, double beatSeconds,
                             int beatsPerBar, const NoteTools::Scale& key, const juce::File& file,
                             const juce::String& title)
{
    const auto xml = toMusicXml (clip, sampleRate, beatSeconds, beatsPerBar, key, title);
    if (! file.replaceWithText (xml))
        return "Could not write MusicXML to " + file.getFileName() + ".";
    return {};
}
}
