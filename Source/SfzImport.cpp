#include "SfzImport.h"

namespace SfzImport
{
namespace
{
    using Opcodes = std::map<juce::String, juce::String>;

    juce::String stripComments (const juce::String& text)
    {
        juce::StringArray lines;
        lines.addLines (text);
        juce::String out;
        for (auto& line : lines)
        {
            const auto pos = line.indexOf ("//");
            out += (pos >= 0 ? line.substring (0, pos) : line) + "\n";
        }
        return out;
    }

    /** Parses opcodes inside a section body (handling spaces inside `sample=` and `default_path=`). */
    Opcodes parseOpcodes (const juce::String& body)
    {
        Opcodes ops;
        const auto len = body.length();
        int i = 0;

        auto isKeyChar = [] (juce::juce_wchar c) noexcept
        {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        };

        // Find all `key=` token starts
        struct Match { int keyStart, eqPos; juce::String key; };
        std::vector<Match> matches;

        while (i < len)
        {
            while (i < len && juce::CharacterFunctions::isWhitespace (body[i]))
                ++i;
            if (i >= len)
                break;

            const auto start = i;
            while (i < len && isKeyChar (body[i]))
                ++i;

            if (i < len && body[i] == '=' && i > start)
            {
                matches.push_back ({ start, i, body.substring (start, i).toLowerCase() });
                ++i;
            }
            else
            {
                while (i < len && ! juce::CharacterFunctions::isWhitespace (body[i]))
                    ++i;
            }
        }

        for (size_t m = 0; m < matches.size(); ++m)
        {
            const auto valStart = matches[m].eqPos + 1;
            const auto valEnd = (m + 1 < matches.size()) ? matches[m + 1].keyStart : len;
            auto val = body.substring (valStart, valEnd).trim();
            // For non-path opcodes, trim at newline if any trailing junk exists
            if (matches[m].key != "sample" && matches[m].key != "default_path")
            {
                const auto ws = val.indexOfAnyOf (" \t\r\n");
                if (ws >= 0)
                    val = val.substring (0, ws);
            }
            else
            {
                const auto nl = val.indexOfAnyOf ("\r\n");
                if (nl >= 0)
                    val = val.substring (0, nl).trim();
            }
            ops[matches[m].key] = val;
        }
        return ops;
    }

    void merge (Opcodes& dst, const Opcodes& src)
    {
        for (const auto& [k, v] : src)
            dst[k] = v;
    }

    int parseIntOrNote (const Opcodes& ops, const juce::String& key, int fallback)
    {
        const auto it = ops.find (key);
        if (it == ops.end())
            return fallback;
        const auto n = parseNote (it->second);
        return n >= 0 ? n : fallback;
    }

    float parseFloat (const Opcodes& ops, const juce::String& key, float fallback)
    {
        const auto it = ops.find (key);
        return it != ops.end() ? it->second.getFloatValue() : fallback;
    }

    juce::int64 parseInt64 (const Opcodes& ops, const juce::String& key, juce::int64 fallback)
    {
        const auto it = ops.find (key);
        return it != ops.end() ? it->second.getLargeIntValue() : fallback;
    }
}

int parseNote (const juce::String& raw) noexcept
{
    const auto s = raw.trim().toLowerCase();
    if (s.isEmpty())
        return -1;

    if ((s[0] >= '0' && s[0] <= '9') || s[0] == '-')
    {
        const auto v = s.getIntValue();
        return (v >= 0 && v <= 127) ? v : -1;
    }

    static constexpr int letterToPc[7] = { 9, 11, 0, 2, 4, 5, 7 }; // a..g
    const auto ch = s[0];
    if (ch < 'a' || ch > 'g')
        return -1;

    int pc = letterToPc[ch - 'a'];
    int idx = 1;
    if (idx < s.length() && (s[idx] == '#' || s[idx] == 's')) { ++pc; ++idx; }
    else if (idx < s.length() && s[idx] == 'b') { --pc; ++idx; }

    if (idx >= s.length())
        return -1;

    const auto oct = s.substring (idx).getIntValue();
    const auto midi = (oct + 1) * 12 + pc;
    return (midi >= 0 && midi <= 127) ? midi : -1;
}

Result parse (const juce::String& sfzText, const juce::File& baseFolder)
{
    Result result;
    const auto clean = stripComments (sfzText);

    // Split into <header> + body chunks
    struct Section { juce::String header, body; };
    std::vector<Section> sections;

    int pos = 0;
    const auto len = clean.length();
    while (pos < len)
    {
        const auto open = clean.indexOfChar (pos, '<');
        if (open < 0)
            break;
        const auto close = clean.indexOfChar (open + 1, '>');
        if (close < 0)
            break;

        const auto hdr = clean.substring (open + 1, close).trim().toLowerCase();
        const auto nextOpen = clean.indexOfChar (close + 1, '<');
        const auto bodyEnd = nextOpen >= 0 ? nextOpen : len;
        sections.push_back ({ hdr, clean.substring (close + 1, bodyEnd) });
        pos = bodyEnd;
    }

    if (sections.empty())
    {
        result.error = "No SFZ sections (<region>, <group>, <global>) found.";
        return result;
    }

    juce::String defaultPath;
    Opcodes globalOps, groupOps;

    for (const auto& sec : sections)
    {
        const auto ops = parseOpcodes (sec.body);

        if (sec.header == "control")
        {
            if (const auto it = ops.find ("default_path"); it != ops.end())
                defaultPath = it->second.replaceCharacter ('\\', '/');
        }
        else if (sec.header == "global")
        {
            globalOps = ops;
            groupOps.clear();
        }
        else if (sec.header == "group")
        {
            groupOps = ops;
        }
        else if (sec.header == "region")
        {
            Opcodes merged = globalOps;
            merge (merged, groupOps);
            merge (merged, ops);

            const auto sampleIt = merged.find ("sample");
            if (sampleIt == merged.end() || sampleIt->second.isEmpty())
                continue;

            auto rel = sampleIt->second.replaceCharacter ('\\', '/');
            if (defaultPath.isNotEmpty() && ! rel.startsWithChar ('/'))
            {
                if (! defaultPath.endsWithChar ('/'))
                    rel = defaultPath + "/" + rel;
                else
                    rel = defaultPath + rel;
            }

            const auto sampleFile = baseFolder.getChildFile (rel);
            juce::String loadErr;
            auto samplePtr = sampleFile.existsAsFile() ? Sampler::sharedSample (sampleFile, loadErr) : Sampler::SamplePtr {};
            if (samplePtr == nullptr)
            {
                ++result.missingSamples;
                continue;
            }

            Sampler::Zone z;
            z.sample = std::move (samplePtr);
            z.file = sampleFile;
            z.name = sampleFile.getFileNameWithoutExtension();

            if (merged.count ("key") > 0)
            {
                const auto k = parseIntOrNote (merged, "key", 60);
                z.lowKey = z.highKey = z.root = k;
            }
            z.lowKey = parseIntOrNote (merged, "lokey", z.lowKey);
            z.highKey = parseIntOrNote (merged, "hikey", z.highKey);
            z.root = parseIntOrNote (merged, "pitch_keycenter", z.root);
            if (z.lowKey > z.highKey)
                std::swap (z.lowKey, z.highKey);

            z.lowVelocity = juce::jlimit (1, 127, juce::roundToInt (parseFloat (merged, "lovel", 1.0f)));
            z.highVelocity = juce::jlimit (1, 127, juce::roundToInt (parseFloat (merged, "hivel", 127.0f)));
            if (z.lowVelocity > z.highVelocity)
                std::swap (z.lowVelocity, z.highVelocity);

            const auto transposeSt = parseFloat (merged, "transpose", 0.0f);
            const auto tuneCents = parseFloat (merged, "tune", 0.0f);
            z.root = juce::jlimit (0, 127, z.root - juce::roundToInt (transposeSt));
            z.tune = juce::jlimit (-100.0f, 100.0f, tuneCents);

            z.level = juce::jlimit (-48.0f, 12.0f, parseFloat (merged, "volume", 0.0f));
            z.pan = juce::jlimit (-100.0f, 100.0f, parseFloat (merged, "pan", 0.0f));

            z.start = juce::jmax ((juce::int64) 0, parseInt64 (merged, "offset", 0));
            const auto endOp = parseInt64 (merged, "end", -1);
            z.end = endOp > 0 ? endOp + 1 : -1;

            if (const auto lm = merged.find ("loop_mode"); lm != merged.end())
            {
                const auto v = lm->second.toLowerCase();
                if (v == "one_shot")
                    z.oneShot = true;
                else if (v == "loop_continuous")
                {
                    z.loop = 1;
                    z.loopInRelease = true;
                }
                else if (v == "loop_sustain")
                {
                    z.loop = 1;
                    z.loopInRelease = false;
                }
            }
            if (merged.count ("loop_start") > 0)
                z.loopStart = juce::jmax ((juce::int64) 0, parseInt64 (merged, "loop_start", 0));
            if (merged.count ("loop_end") > 0)
                z.loopEnd = parseInt64 (merged, "loop_end", -1) + 1;

            if (const auto offBy = parseInt64 (merged, "off_by", 0); offBy > 0)
                z.choke = juce::jlimit (1, 16, (int) offBy);
            else if (const auto grp = parseInt64 (merged, "group", 0); grp > 0 && merged.count ("off_by") > 0)
                z.choke = juce::jlimit (1, 16, (int) grp);

            if (const auto seqLen = parseInt64 (merged, "seq_length", 0); seqLen > 1)
                z.roundRobinGroup = juce::jlimit (1, 16, (int) std::max<juce::int64> (1, parseInt64 (merged, "group", 1)));

            if (! result.hasEnvelope && (merged.count ("ampeg_attack") || merged.count ("ampeg_decay") || merged.count ("ampeg_sustain") || merged.count ("ampeg_release")))
            {
                result.hasEnvelope = true;
                result.settings.attack  = juce::jlimit (0.0f, 10000.0f, parseFloat (merged, "ampeg_attack", 0.001f) * 1000.0f);
                result.settings.decay   = juce::jlimit (0.0f, 10000.0f, parseFloat (merged, "ampeg_decay", 0.3f) * 1000.0f);
                result.settings.sustain = juce::jlimit (0.0f, 100.0f, parseFloat (merged, "ampeg_sustain", 100.0f));
                result.settings.release = juce::jlimit (0.0f, 10000.0f, parseFloat (merged, "ampeg_release", 0.1f) * 1000.0f);
            }

            result.zones.push_back (std::move (z));
            if ((int) result.zones.size() >= Sampler::maxZones)
                break;
        }
    }

    if (result.zones.empty())
        result.error = result.missingSamples > 0
            ? ("None of the SFZ samples could be loaded (" + juce::String (result.missingSamples) + " missing).")
            : "The SFZ file contains no playable regions.";

    return result;
}

Result loadFile (const juce::File& sfzFile)
{
    Result r;
    if (! sfzFile.existsAsFile())
    {
        r.error = "SFZ file not found: " + sfzFile.getFullPathName();
        return r;
    }
    return parse (sfzFile.loadFileAsString(), sfzFile.getParentDirectory());
}
}
