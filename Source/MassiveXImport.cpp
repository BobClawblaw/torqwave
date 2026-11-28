#include "MassiveXImport.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>

namespace MassiveXImport
{
namespace
{
    using juce::uint8;
    using juce::uint16;
    using juce::uint32;
    using juce::uint64;

    constexpr size_t maxDecompressedBytes = 32 * 1024 * 1024; // 32 MB safety cap
    constexpr int maxMsgPackDepth = 16;

    //==========================================================================
    // Bounds-checked minimal MessagePack reader (zero external dependencies)
    struct MpReader
    {
        const uint8* data = nullptr;
        size_t size = 0;
        size_t pos = 0;
        bool ok = true;

        bool need (size_t n) noexcept
        {
            if (! ok || n > size - pos)
            {
                ok = false;
                return false;
            }
            return true;
        }

        uint8 u8() noexcept { return need (1) ? data[pos++] : (uint8) 0; }

        uint16 u16be() noexcept
        {
            if (! need (2)) return 0;
            const uint16 v = (uint16) (((uint16) data[pos] << 8) | data[pos + 1]);
            pos += 2;
            return v;
        }

        uint32 u32be() noexcept
        {
            if (! need (4)) return 0;
            const uint32 v = ((uint32) data[pos] << 24) | ((uint32) data[pos + 1] << 16)
                           | ((uint32) data[pos + 2] << 8) | (uint32) data[pos + 3];
            pos += 4;
            return v;
        }

        uint64 u64be() noexcept
        {
            const uint64 hi = u32be();
            const uint64 lo = u32be();
            return (hi << 32) | lo;
        }

        juce::String readStrBytes (size_t n)
        {
            if (! need (n)) return {};
            juce::String s = juce::String::fromUTF8 ((const char*) (data + pos), (int) std::min<size_t> (n, 65536));
            pos += n;
            return s;
        }

        bool readString (juce::String& out)
        {
            if (! need (1)) return false;
            const uint8 b = data[pos++];
            size_t len = 0;
            if ((b & 0xe0) == 0xa0)      len = b & 0x1f;
            else if (b == 0xd9)          len = u8();
            else if (b == 0xda)          len = u16be();
            else if (b == 0xdb)          len = u32be();
            else { ok = false; return false; }
            out = readStrBytes (len);
            return ok;
        }

        bool readInt (int64_t& out)
        {
            if (! need (1)) return false;
            const uint8 b = data[pos++];
            if (b <= 0x7f)          { out = (int64_t) b; return true; }
            if (b >= 0xe0)          { out = (int64_t) (int8_t) b; return true; }
            if (b == 0xcc)          { out = (int64_t) u8(); return ok; }
            if (b == 0xcd)          { out = (int64_t) u16be(); return ok; }
            if (b == 0xce)          { out = (int64_t) u32be(); return ok; }
            if (b == 0xcf)          { out = (int64_t) u64be(); return ok; }
            if (b == 0xd0)          { out = (int64_t) (int8_t) u8(); return ok; }
            if (b == 0xd1)          { out = (int64_t) (int16_t) u16be(); return ok; }
            if (b == 0xd2)          { out = (int64_t) (int32_t) u32be(); return ok; }
            if (b == 0xd3)          { out = (int64_t) u64be(); return ok; }
            ok = false;
            return false;
        }

        bool readNumber (double& out)
        {
            if (! need (1)) return false;
            const uint8 b = data[pos];
            if (b == 0xca)
            {
                ++pos;
                const uint32 bits = u32be();
                float f = 0.0f;
                std::memcpy (&f, &bits, 4);
                out = (double) f;
                return ok;
            }
            if (b == 0xcb)
            {
                ++pos;
                const uint64 bits = u64be();
                double d = 0.0;
                std::memcpy (&d, &bits, 8);
                out = d;
                return ok;
            }
            int64_t iv = 0;
            if (! readInt (iv)) return false;
            out = (double) iv;
            return true;
        }

        bool readBool (bool& out)
        {
            if (! need (1)) return false;
            const uint8 b = data[pos++];
            if (b == 0xc2) { out = false; return true; }
            if (b == 0xc3) { out = true; return true; }
            ok = false;
            return false;
        }

        bool readMapHeader (uint32& count)
        {
            if (! need (1)) return false;
            const uint8 b = data[pos++];
            if ((b & 0xf0) == 0x80)      { count = b & 0x0f; return true; }
            if (b == 0xde)               { count = u16be(); return ok; }
            if (b == 0xdf)               { count = u32be(); return ok; }
            ok = false;
            return false;
        }

        bool readArrayHeader (uint32& count)
        {
            if (! need (1)) return false;
            const uint8 b = data[pos++];
            if ((b & 0xf0) == 0x90)      { count = b & 0x0f; return true; }
            if (b == 0xdc)               { count = u16be(); return ok; }
            if (b == 0xdd)               { count = u32be(); return ok; }
            ok = false;
            return false;
        }

        bool skipValue (int depth = 0)
        {
            if (depth > maxMsgPackDepth || ! need (1)) { ok = false; return false; }
            const uint8 b = data[pos++];
            if (b <= 0x7f || b >= 0xe0 || b == 0xc0 || b == 0xc2 || b == 0xc3) return true;
            if ((b & 0xe0) == 0xa0) return need (b & 0x1f) && ((pos += (b & 0x1f)), true);
            if ((b & 0xf0) == 0x90)
            {
                const uint32 n = b & 0x0f;
                for (uint32 i = 0; i < n; ++i) if (! skipValue (depth + 1)) return false;
                return true;
            }
            if ((b & 0xf0) == 0x80)
            {
                const uint32 n = b & 0x0f;
                for (uint32 i = 0; i < n; ++i) if (! skipValue (depth + 1) || ! skipValue (depth + 1)) return false;
                return true;
            }
            switch (b)
            {
                case 0xc4: { const size_t n = u8();    return need (n) && ((pos += n), true); }
                case 0xc5: { const size_t n = u16be(); return need (n) && ((pos += n), true); }
                case 0xc6: { const size_t n = u32be(); return need (n) && ((pos += n), true); }
                case 0xc7: { const size_t n = u8();    return need (n + 1) && ((pos += n + 1), true); }
                case 0xc8: { const size_t n = u16be(); return need (n + 1) && ((pos += n + 1), true); }
                case 0xc9: { const size_t n = u32be(); return need (n + 1) && ((pos += n + 1), true); }
                case 0xca: case 0xce: case 0xd2: return need (4) && ((pos += 4), true);
                case 0xcb: case 0xcf: case 0xd3: return need (8) && ((pos += 8), true);
                case 0xcc: case 0xd0:            return need (1) && ((pos += 1), true);
                case 0xcd: case 0xd1:            return need (2) && ((pos += 2), true);
                case 0xd4: return need (2) && ((pos += 2), true);
                case 0xd5: return need (3) && ((pos += 3), true);
                case 0xd6: return need (5) && ((pos += 5), true);
                case 0xd7: return need (9) && ((pos += 9), true);
                case 0xd8: return need (17) && ((pos += 17), true);
                case 0xd9: { const size_t n = u8();    return need (n) && ((pos += n), true); }
                case 0xda: { const size_t n = u16be(); return need (n) && ((pos += n), true); }
                case 0xdb: { const size_t n = u32be(); return need (n) && ((pos += n), true); }
                case 0xdc: case 0xdd:
                {
                    const uint32 n = (b == 0xdc) ? (uint32) u16be() : u32be();
                    if (! ok || n > 4000000) { ok = false; return false; }
                    for (uint32 i = 0; i < n; ++i) if (! skipValue (depth + 1)) return false;
                    return true;
                }
                case 0xde: case 0xdf:
                {
                    const uint32 n = (b == 0xde) ? (uint32) u16be() : u32be();
                    if (! ok || n > 500000) { ok = false; return false; }
                    for (uint32 i = 0; i < n; ++i) if (! skipValue (depth + 1) || ! skipValue (depth + 1)) return false;
                    return true;
                }
                default:
                    ok = false;
                    return false;
            }
        }
    };

    //==========================================================================
    // Zlib helper using JUCE's built-in GZIPDecompressorInputStream
    bool inflateZlib (const uint8* src, size_t srcLen, juce::MemoryBlock& dest, size_t maxBytes = maxDecompressedBytes)
    {
        if (src == nullptr || srcLen < 6)
            return false;

        juce::MemoryInputStream mis (src, srcLen, false);
        juce::GZIPDecompressorInputStream gz (&mis, false, juce::GZIPDecompressorInputStream::zlibFormat);

        dest.reset();
        uint8 chunk[16384];
        size_t total = 0;

        while (! gz.isExhausted())
        {
            const int got = gz.read (chunk, (int) sizeof (chunk));
            if (got <= 0)
                break;
            total += (size_t) got;
            if (total > maxBytes)
                return false;
            dest.append (chunk, (size_t) got);
        }

        return dest.getSize() > 0;
    }

    //==========================================================================
    // Parsed Massive X parameter tree
    struct MxTree
    {
        std::map<juce::String, juce::String> strings;
        std::map<juce::String, double> doubles;
        std::map<juce::String, int64_t> ints;
        std::map<juce::String, bool> bools;
        std::map<juce::String, std::vector<int64_t>> intVecs;

        juce::String str (const juce::String& key, const juce::String& fallback = {}) const
        {
            if (const auto it = strings.find (key); it != strings.end())
                return it->second;
            return fallback;
        }

        double num (const juce::String& key, double fallback = 0.0) const
        {
            if (const auto it = doubles.find (key); it != doubles.end())
                return it->second;
            return fallback;
        }

        int64_t integer (const juce::String& key, int64_t fallback = 0) const
        {
            if (const auto it = ints.find (key); it != ints.end())
                return it->second;
            return fallback;
        }

        bool flag (const juce::String& key, bool fallback = false) const
        {
            if (const auto it = bools.find (key); it != bools.end())
                return it->second;
            return fallback;
        }
    };

    bool parsePchkTree (const uint8* data, size_t size, MxTree& out)
    {
        MpReader r { data, size, 0, true };

        while (r.ok && r.pos < r.size)
        {
            juce::String section;
            int64_t marker = 0;
            if (! r.readString (section) || ! r.readInt (marker))
                break;

            auto nextIsMap = [&r]() noexcept
            {
                if (r.pos >= r.size) return false;
                const uint8 nb = r.data[r.pos];
                return (nb & 0xf0) == 0x80 || nb == 0xde || nb == 0xdf;
            };

            if (section == "strings" || section == "floats" || section == "doubles" || section == "ints" || section == "bools")
                if (! nextIsMap())
                    continue;

            if (section == "strings")
            {
                uint32 n = 0;
                if (! r.readMapHeader (n) || n > 100000) return false;
                for (uint32 i = 0; i < n && r.ok; ++i)
                {
                    juce::String k, v;
                    if (! r.readString (k)) return false;
                    if (r.pos < r.size && ((r.data[r.pos] & 0xe0) == 0xa0 || r.data[r.pos] == 0xd9 || r.data[r.pos] == 0xda || r.data[r.pos] == 0xdb))
                    {
                        if (! r.readString (v)) return false;
                        out.strings[k] = v;
                    }
                    else
                    {
                        if (! r.skipValue()) return false;
                    }
                }
            }
            else if (section == "floats" || section == "doubles")
            {
                uint32 n = 0;
                if (! r.readMapHeader (n) || n > 100000) return false;
                for (uint32 i = 0; i < n && r.ok; ++i)
                {
                    juce::String k;
                    double v = 0.0;
                    if (! r.readString (k) || ! r.readNumber (v)) return false;
                    if (std::isfinite (v))
                        out.doubles[k] = v;
                }
            }
            else if (section == "ints")
            {
                uint32 n = 0;
                if (! r.readMapHeader (n) || n > 100000) return false;
                for (uint32 i = 0; i < n && r.ok; ++i)
                {
                    juce::String k;
                    int64_t v = 0;
                    if (! r.readString (k) || ! r.readInt (v)) return false;
                    out.ints[k] = v;
                }
            }
            else if (section == "bools")
            {
                uint32 n = 0;
                if (! r.readMapHeader (n) || n > 100000) return false;
                for (uint32 i = 0; i < n && r.ok; ++i)
                {
                    juce::String k;
                    bool v = false;
                    if (! r.readString (k) || ! r.readBool (v)) return false;
                    out.bools[k] = v;
                }
            }
            else if (section == "intVecs")
            {
                int64_t count = 0;
                if (! r.readInt (count) || count < 0 || count > 2000)
                    break;
                for (int64_t i = 0; i < count && r.ok; ++i)
                {
                    juce::String k;
                    uint32 len = 0;
                    if (! r.readString (k) || ! r.readArrayHeader (len) || len > 100000) return false;
                    const bool keep = k.contains ("/page 0/points");
                    std::vector<int64_t> vec;
                    if (keep) vec.reserve (len);
                    for (uint32 j = 0; j < len && r.ok; ++j)
                    {
                        int64_t v = 0;
                        if (! r.readInt (v)) return false;
                        if (keep) vec.push_back (v);
                    }
                    if (keep) out.intVecs[k] = std::move (vec);
                }
            }
            else
            {
                // Remaining sections (charVecs, stringVecs, etc.) not needed for patch translation
                break;
            }
        }

        return ! out.doubles.empty() || ! out.strings.empty();
    }

    //==========================================================================
    // NISI & PLID parsers
    void parseNisi (const uint8* data, size_t size, PresetInfo& info)
    {
        if (size <= 4) return;
        MpReader r { data + 4, size - 4, 0, true };
        uint32 n = 0;
        if (! r.readMapHeader (n) || n > 256) return;

        for (uint32 i = 0; i < n && r.ok; ++i)
        {
            juce::String key;
            if (! r.readString (key)) break;

            if (key == "name")         { r.readString (info.name); }
            else if (key == "author")  { r.readString (info.author); }
            else if (key == "comment") { r.readString (info.comment); }
            else if (key == "bankchain")
            {
                uint32 len = 0;
                if (r.readArrayHeader (len) && len <= 16)
                {
                    for (uint32 j = 0; j < len && r.ok; ++j)
                    {
                        juce::String s;
                        if (r.readString (s) && s.isNotEmpty() && info.bank.isEmpty() && j > 0)
                            info.bank = s;
                    }
                }
            }
            else if (key == "characters")
            {
                uint32 len = 0;
                if (r.readArrayHeader (len) && len <= 64)
                {
                    for (uint32 j = 0; j < len && r.ok; ++j)
                    {
                        juce::String s;
                        if (r.readString (s) && s.isNotEmpty())
                            info.characters.addIfNotAlreadyThere (s);
                    }
                }
            }
            else if (key == "types")
            {
                uint32 len = 0;
                if (r.readArrayHeader (len) && len <= 64)
                {
                    for (uint32 j = 0; j < len && r.ok; ++j)
                    {
                        uint32 subLen = 0;
                        if (r.readArrayHeader (subLen) && subLen <= 8)
                        {
                            for (uint32 k = 0; k < subLen && r.ok; ++k)
                            {
                                juce::String s;
                                if (r.readString (s) && s.isNotEmpty())
                                    info.types.addIfNotAlreadyThere (s);
                            }
                        }
                    }
                }
            }
            else
            {
                r.skipValue();
            }
        }

        if (! info.types.isEmpty())
            info.category = info.types[0];
    }

    bool parsePlidIsMassiveX (const uint8* data, size_t size)
    {
        if (size <= 4) return false;
        MpReader r { data + 4, size - 4, 0, true };
        uint32 n = 0;
        if (! r.readMapHeader (n) || n > 64) return false;

        bool match = false;
        for (uint32 i = 0; i < n && r.ok; ++i)
        {
            juce::String key;
            if (! r.readString (key)) break;
            if (key == "VST.magic")
            {
                int64_t magic = 0;
                if (r.readInt (magic) && magic == 0x4e692448) // 'Ni$H' = Massive X
                    match = true;
            }
            else if (key == "pluginName")
            {
                juce::String pname;
                if (r.readString (pname) && pname.containsIgnoreCase ("Massive X"))
                    match = true;
            }
            else
            {
                r.skipValue();
            }
        }
        return match;
    }

    //==========================================================================
    // Local user-owned .mxwt wavetable index & single-cycle FFT loader
    // Strictly reads ONLY files already present on the user's machine; never bundles NI data.
    juce::String stripHyphensLower (const juce::String& s)
    {
        return s.removeCharacters ("-").trim().toLowerCase();
    }

    std::mutex wtCacheMutex;
    juce::String cachedWtFolder;
    std::map<juce::String, juce::File> cachedWtByHash;
    std::map<juce::String, std::shared_ptr<const WaveTables::Sampled>> cachedSampledByHash;

    juce::String readMxwtHash (const juce::File& file)
    {
        juce::FileInputStream fis (file);
        if (! fis.openedOk() || fis.getTotalLength() < 16)
            return {};

        uint8 compressed[4096];
        const int compRead = fis.read (compressed, (int) sizeof (compressed));
        if (compRead < 16)
            return {};

        // Decompress just the first 2 KB to read meta/hash quickly without loading multi-MB tables
        juce::MemoryInputStream mis (compressed + 4, (size_t) (compRead - 4), false);
        juce::GZIPDecompressorInputStream gz (&mis, false, juce::GZIPDecompressorInputStream::zlibFormat);
        uint8 head[2048];
        const int got = gz.read (head, (int) sizeof (head));
        if (got <= 16)
            return {};

        MpReader r { head, (size_t) got, 0, true };
        juce::String sec;
        int64_t marker = 0;
        uint32 n = 0;
        if (! r.readString (sec) || sec != "strings" || ! r.readInt (marker) || marker <= 1 || ! r.readMapHeader (n) || n > 64)
            return {};

        for (uint32 i = 0; i < n && r.ok; ++i)
        {
            juce::String k, v;
            if (! r.readString (k) || ! r.readString (v)) break;
            if (k == "meta/hash")
                return stripHyphensLower (v);
        }
        return {};
    }

    std::shared_ptr<const WaveTables::Sampled> loadLocalMxwt (const juce::String& rawHash, const juce::File& folder)
    {
        const auto hash = stripHyphensLower (rawHash);
        if (hash.isEmpty() || ! folder.isDirectory())
            return nullptr;

        juce::File targetFile;
        {
            const std::lock_guard<std::mutex> lock (wtCacheMutex);
            if (cachedWtFolder != folder.getFullPathName())
            {
                cachedWtFolder = folder.getFullPathName();
                cachedWtByHash.clear();
                cachedSampledByHash.clear();

                for (const auto& entry : juce::RangedDirectoryIterator (folder, true, "*.mxwt", juce::File::findFiles))
                {
                    const auto h = readMxwtHash (entry.getFile());
                    if (h.isNotEmpty())
                        cachedWtByHash[h] = entry.getFile();
                }
            }

            if (const auto hit = cachedSampledByHash.find (hash); hit != cachedSampledByHash.end())
                return hit->second;

            if (const auto fIt = cachedWtByHash.find (hash); fIt != cachedWtByHash.end())
                targetFile = fIt->second;
            else
                return nullptr;
        }

        // Load and decompress the single requested local .mxwt file
        juce::MemoryBlock raw, dec;
        if (! targetFile.loadFileAsData (raw) || raw.getSize() < 16)
            return nullptr;

        if (! inflateZlib ((const uint8*) raw.getData() + 4, raw.getSize() - 4, dec, 24 * 1024 * 1024))
            return nullptr;

        MpReader r { (const uint8*) dec.getData(), dec.getSize(), 0, true };
        juce::String tableName = targetFile.getFileNameWithoutExtension();
        int wtEntries = 0;
        uint32 wtTotalLen = 0;
        size_t wtDataStart = 0;

        while (r.ok && r.pos < r.size)
        {
            juce::String sec;
            int64_t marker = 0;
            if (! r.readString (sec) || ! r.readInt (marker))
                break;

            if (marker <= 1)
                continue;

            if (sec == "strings")
            {
                uint32 n = 0;
                if (! r.readMapHeader (n) || n > 128) break;
                for (uint32 i = 0; i < n && r.ok; ++i)
                {
                    juce::String k, v;
                    if (! r.readString (k) || ! r.readString (v)) break;
                    if (k == "meta/name" && v.isNotEmpty()) tableName = v;
                }
            }
            else if (sec == "ints")
            {
                uint32 n = 0;
                if (! r.readMapHeader (n) || n > 128) break;
                for (uint32 i = 0; i < n && r.ok; ++i)
                {
                    juce::String k;
                    int64_t v = 0;
                    if (! r.readString (k) || ! r.readInt (v)) break;
                    if (k == "root/wtEntries") wtEntries = (int) v;
                }
            }
            else if (sec == "floats" || sec == "doubles" || sec == "bools")
            {
                if (! r.skipValue()) break;
            }
            else if (sec == "charVecs" || sec == "intVecs" || sec == "doubleVecs" || sec == "stringVecs")
            {
                int64_t cnt = 0;
                if (! r.readInt (cnt) || cnt < 0 || cnt > 512) break;
                for (int64_t i = 0; i < cnt && r.ok; ++i)
                {
                    if (! r.skipValue() || ! r.skipValue()) break;
                }
            }
            else if (sec == "floatVecs")
            {
                int64_t cnt = 0;
                if (! r.readInt (cnt) || cnt < 0 || cnt > 64) break;
                for (int64_t i = 0; i < cnt && r.ok; ++i)
                {
                    juce::String k;
                    if (! r.readString (k)) break;
                    if (k == "root/wtData")
                    {
                        uint32 len = 0;
                        if (! r.readArrayHeader (len) || len < 2048 || len > 4000000) break;
                        wtTotalLen = len;
                        wtDataStart = r.pos;
                        break;
                    }
                    else
                    {
                        if (! r.skipValue()) break;
                    }
                }
                break;
            }
            else
            {
                break;
            }
        }

        if (wtEntries <= 0 || wtTotalLen < 2048 || wtDataStart == 0)
            return nullptr;

        const size_t stride = (size_t) wtTotalLen / (size_t) wtEntries;
        if (stride < 2048)
            return nullptr;

        constexpr int fftOrder = 11; // 2048 samples
        constexpr int fftSize = 1 << fftOrder;
        juce::dsp::FFT fft (fftOrder);
        std::vector<float> fftBuf ((size_t) fftSize * 2, 0.0f);

        const int outFrames = std::clamp (wtEntries, 2, 32);
        std::vector<float> amplitudes ((size_t) outFrames * (size_t) WaveTables::Sampled::harmonics, 0.0f);

        // Fast path: packed 5-byte msgpack float32 elements (0xca + 4-byte BE float) -> O(1) seek per frame!
        const bool packedFloat32 = (wtDataStart + (size_t) wtTotalLen * 5 <= r.size) && (r.data[wtDataStart] == 0xca);
        std::vector<float> fallbackAll;
        if (! packedFloat32)
        {
            r.pos = wtDataStart;
            fallbackAll.resize (wtTotalLen, 0.0f);
            for (uint32 j = 0; j < wtTotalLen && r.ok; ++j)
            {
                double fv = 0.0;
                if (! r.readNumber (fv)) break;
                fallbackAll[j] = (float) fv;
            }
        }

        for (int f = 0; f < outFrames; ++f)
        {
            const int srcIdx = (wtEntries == 1) ? 0
                             : std::clamp ((int) std::lround ((double) f * (double) (wtEntries - 1) / (double) (outFrames - 1)), 0, wtEntries - 1);

            std::fill (fftBuf.begin(), fftBuf.end(), 0.0f);
            if (packedFloat32)
            {
                const size_t baseOff = wtDataStart + (size_t) srcIdx * stride * 5;
                for (int s = 0; s < fftSize; ++s)
                {
                    const uint8* p = r.data + baseOff + (size_t) s * 5;
                    if (p[0] != 0xca) break;
                    const uint32 bits = ((uint32) p[1] << 24) | ((uint32) p[2] << 16) | ((uint32) p[3] << 8) | (uint32) p[4];
                    float fv = 0.0f;
                    std::memcpy (&fv, &bits, 4);
                    fftBuf[(size_t) s] = std::isfinite (fv) ? fv : 0.0f;
                }
            }
            else
            {
                const float* cycle = fallbackAll.data() + (size_t) srcIdx * stride;
                std::memcpy (fftBuf.data(), cycle, (size_t) fftSize * sizeof (float));
            }

            fft.performRealOnlyForwardTransform (fftBuf.data(), true);

            float* dst = amplitudes.data() + (size_t) f * (size_t) WaveTables::Sampled::harmonics;
            for (int h = 1; h <= WaveTables::Sampled::harmonics; ++h)
            {
                const float re = fftBuf[(size_t) h * 2];
                const float im = fftBuf[(size_t) h * 2 + 1];
                dst[h - 1] = std::hypot (re, im);
            }
        }

        auto sampled = WaveTables::makeSampled (amplitudes, outFrames, 261.625565, tableName);
        if (sampled != nullptr)
        {
            const std::lock_guard<std::mutex> lock (wtCacheMutex);
            cachedSampledByHash[hash] = sampled;
        }
        return sampled;
    }

    //==========================================================================
    // Clean-room parameter mapping helpers (Massive X -> WaveSynth)
    double normToMs (double x, double maxMs = 6000.0) noexcept
    {
        const double c = std::clamp (x, 0.0, 1.0);
        return std::clamp (maxMs * std::pow (c, 3.0), 0.0, 10000.0);
    }

    double normToCutoffHz (double x) noexcept
    {
        const double c = std::clamp (x, 0.0, 1.0);
        return std::clamp (20.0 * std::pow (1000.0, c), 20.0, 20000.0);
    }

    int mapWarpMode (const juce::String& mode) noexcept
    {
        if (mode.equalsIgnoreCase ("Hardsync")) return (int) WaveSynth::Warp::sync;
        if (mode.equalsIgnoreCase ("Bend"))     return (int) WaveSynth::Warp::bend;
        if (mode.equalsIgnoreCase ("Mirror"))   return (int) WaveSynth::Warp::mirror;
        if (mode.equalsIgnoreCase ("Formant"))  return (int) WaveSynth::Warp::squeeze;
        if (mode.equalsIgnoreCase ("Wrap") || mode.equalsIgnoreCase ("Gorilla")) return (int) WaveSynth::Warp::fold;
        if (mode.equalsIgnoreCase ("Jitter") || mode.equalsIgnoreCase ("Random")) return (int) WaveSynth::Warp::crush;
        return (int) WaveSynth::Warp::off;
    }

    int pickFallbackTable (const PresetInfo& info, const juce::String& warpMode, int oscIdx) noexcept
    {
        const auto allTags = (info.types.joinIntoString (" ") + " " + info.characters.joinIntoString (" ")).toLowerCase();
        if (warpMode.equalsIgnoreCase ("Formant") || allTags.contains ("vocal") || allTags.contains ("vox"))
            return (int) WaveTables::Table::vowel;
        if (allTags.contains ("metallic") || allTags.contains ("bell") || allTags.contains ("perc"))
            return (int) WaveTables::Table::metal;
        if (allTags.contains ("fm") || allTags.contains ("digital") || allTags.contains ("glass"))
            return (int) WaveTables::Table::fm;
        if (allTags.contains ("growl") || allTags.contains ("distort") || allTags.contains ("acid") || allTags.contains ("dirty"))
            return (int) WaveTables::Table::growl;
        if (allTags.contains ("sub") && oscIdx == 0)
            return (int) WaveTables::Table::sub;
        if (allTags.contains ("organ") || allTags.contains ("additive") || allTags.contains ("choir"))
            return (int) WaveTables::Table::additive;
        if (allTags.contains ("pluck") || allTags.contains ("guitar"))
            return (int) WaveTables::Table::pulse;
        return oscIdx == 0 ? (int) WaveTables::Table::analog : (int) WaveTables::Table::pulse;
    }

    int mapFilterType (const juce::String& plugin, const juce::String& subType) noexcept
    {
        if (plugin.containsIgnoreCase ("Comb"))
            return (int) WaveSynth::FilterType::comb;
        if (plugin.containsIgnoreCase ("SV2") || plugin.containsIgnoreCase ("SVParallel") || plugin.containsIgnoreCase ("Asimov"))
        {
            if (subType.containsIgnoreCase ("HP")) return (int) WaveSynth::FilterType::svfHigh;
            if (subType.containsIgnoreCase ("BP")) return (int) WaveSynth::FilterType::svfBand;
            if (subType.containsIgnoreCase ("Notch") || subType.containsIgnoreCase ("BR")) return (int) WaveSynth::FilterType::svfNotch;
            return (int) WaveSynth::FilterType::svfLow;
        }
        if (subType.containsIgnoreCase ("HP")) return (int) WaveSynth::FilterType::ladderHigh;
        if (subType.containsIgnoreCase ("BP")) return (int) WaveSynth::FilterType::ladderBand;
        if (subType.containsIgnoreCase ("12") || subType.containsIgnoreCase ("LP2")) return (int) WaveSynth::FilterType::ladder12;
        return (int) WaveSynth::FilterType::ladder24;
    }

    int destinationIndexFor (const juce::String& destId)
    {
        const auto& ids = WaveSynth::destinationIds();
        const int idx = ids.indexOf (destId);
        return idx >= 0 ? idx + 1 : 0; // +1 because choice 0 is "None"
    }

    juce::String mapTargetPathToDestId (const juce::String& key)
    {
        if (key.contains ("FilterSlot/") && key.contains ("/Cutoff/"))  return "filter1Cutoff";
        if (key.contains ("FilterSlot/") && key.contains ("/Res/"))     return "filter1Resonance";
        if (key.contains ("FilterSlot/") && key.contains ("/Drive/"))   return "filter1Drive";
        if (key.contains ("MainOsc1Slot/") && key.contains ("/Pos/"))   return "osc1Position";
        if (key.contains ("MainOsc1Slot/") && key.contains ("/Pitch/")) return "osc1Pitch";
        if (key.contains ("MainOsc1Slot/") && key.contains ("/Level/")) return "osc1Volume";
        if (key.contains ("MainOsc1Slot/") && key.contains ("/Alg"))    return "osc1WarpAmount";
        if (key.contains ("MainOsc2Slot/") && key.contains ("/Pos/"))   return "osc2Position";
        if (key.contains ("MainOsc2Slot/") && key.contains ("/Pitch/")) return "osc2Pitch";
        if (key.contains ("MainOsc2Slot/") && key.contains ("/Level/")) return "osc2Volume";
        if (key.contains ("MainOsc2Slot/") && key.contains ("/Alg"))    return "osc2WarpAmount";
        if (key.contains ("Noise1Slot/") && key.contains ("/Level/"))   return "noiseLevel";
        if (key.contains ("NotePitchPlugin/Pitch/"))                    return "osc1Pitch";
        if (key.contains ("PolyphonicMixdown/Pan/"))                    return "ampPan";
        if (key.contains ("output/Level/") || key.contains ("PolyphonicMixdown/Level/")) return "ampLevel";
        if (key.contains ("EcoReverb/DW/"))                             return "fxReverbMix";
        if (key.contains ("StereoDelay/DW/"))                           return "fxDelayMix";
        if (key.contains ("StereoDelay/Feed/"))                         return "fxDelayFeedback";
        if (key.contains ("QuadChorus/DW/"))                            return "fxChorusMix";
        if (key.contains ("Phaser/DW/"))                                return "fxPhaserMix";
        if (key.contains ("GenDistPlugin/Drive/"))                      return "fxDriveAmount";
        return {};
    }

    void translateTree (const MxTree& t, const juce::File& wtFolder, ImportResult& out)
    {
        auto& v = out.values;
        const juce::String u1 = "root/engine/unit1/";

        // Defaults: start clean so imported patch is predictable
        v["ampLevel"] = -9.0;
        v["ampVelocity"] = 60.0;
        v["filterRouting"] = 0.0;
        v["filter2Type"] = (double) (int) WaveSynth::FilterType::off;
        for (int o = 1; o <= WaveSynth::numOscillators; ++o)
            v["osc" + juce::String (o) + "On"] = 0.0;
        for (int s = 1; s <= WaveSynth::numSlots; ++s)
        {
            v["mod" + juce::String (s) + "Source"] = 0.0;
            v["mod" + juce::String (s) + "Destination"] = 0.0;
            v["mod" + juce::String (s) + "Amount"] = 0.0;
        }

        // 1. Voice mode, Unison & Glide
        const int voiceAlloc = (int) t.integer (u1 + "genericAdapters/VoicePage/VoiceAllocation/VoiceAllocationEntry/indexState/value", 0);
        v["voiceMode"] = (voiceAlloc == 1) ? 1.0 : (voiceAlloc == 2 ? 2.0 : 0.0); // Poly / Mono / Legato
        const int unisonIdx = (int) t.integer (u1 + "genericAdapters/VoicePage/UnisonVoices/UnisonVoices_1/indexState/value", 0);
        const int unisonVoices = std::clamp (unisonIdx + 1, 1, WaveSynth::maxUnison);
        const double unisonSpread = std::clamp (t.num (u1 + "audioPluginSlots/NotePitchSlot/audioPlugins/NotePitchPlugin/Spread/Knob/parameterState/normalizedValue", 0.25), 0.0, 1.0);
        const double unisonDetune = std::clamp (t.num (u1 + "audioPluginSlots/NotePitchSlot/audioPlugins/NotePitchPlugin/ChordDetune/Knob/parameterState/normalizedValue", 0.12), 0.0, 1.0);
        const double glideNorm = std::clamp (t.num (u1 + "audioPluginSlots/NotePitchSlot/audioPlugins/NotePitchPlugin/Glide/Knob/parameterState/normalizedValue", 0.0), 0.0, 1.0);
        v["glideTime"] = glideNorm > 0.01 ? normToMs (glideNorm, 1200.0) : 0.0;

        // 2. Main Oscillators 1 & 2
        for (int o = 0; o < 2; ++o)
        {
            const juce::String slot = "MainOsc" + juce::String (o + 1) + "Slot";
            const juce::String pref = u1 + "audioPluginSlots/" + slot + "/audioPlugins/Wavetable/";
            const juce::String oid = "osc" + juce::String (o + 1);

            const double level = std::clamp (t.num (pref + "Level/Knob/parameterState/normalizedValue", o == 0 ? 1.0 : 0.0), 0.0, 1.0);
            const bool active = level > 0.005;
            v[oid + "On"] = active ? 1.0 : 0.0;
            if (! active) continue;

            ++out.oscsMapped;
            const juce::String wtHash = t.str (pref + "wavetable/hash");
            const juce::String warpMode = t.str (pref + "Mode/selectedEntryName", "Standard");

            if (auto sampled = loadLocalMxwt (wtHash, wtFolder))
            {
                out.sampledTables[(size_t) o] = std::move (sampled);
                v[oid + "Table"] = (double) (int) WaveTables::Table::sampled;
                ++out.localWavetablesLoaded;
            }
            else
            {
                v[oid + "Table"] = (double) pickFallbackTable (out.info, warpMode, o);
            }

            const double pos = std::clamp (t.num (pref + "Pos/Knob/parameterState/normalizedValue", 0.5), 0.0, 1.0);
            v[oid + "Position"] = pos * 100.0;

            const int warpIdx = mapWarpMode (warpMode);
            v[oid + "Warp"] = (double) warpIdx;
            if (warpIdx != (int) WaveSynth::Warp::off)
            {
                const double alg = std::clamp (t.num (pref + "Alg2/Alg2_3/Knob/parameterState/normalizedValue", 0.35), 0.0, 1.0);
                v[oid + "WarpAmount"] = alg * 100.0;
            }

            // Massive X pitch rawValue is in milli-semitones (-12000 = -12 st)
            const int64_t rawPitch = t.integer (pref + "Pitch/TuningKnob/tuningState/rawValue", 0);
            const double semisTotal = (double) rawPitch / 1000.0;
            const double semisInt = std::clamp (std::round (semisTotal), -24.0, 24.0);
            const double cents = std::clamp ((semisTotal - semisInt) * 100.0, -100.0, 100.0);
            v[oid + "Pitch"] = semisInt;
            v[oid + "Fine"] = cents;
            v[oid + "Volume"] = level * 95.0;
            v[oid + "Unison"] = (double) unisonVoices;
            v[oid + "UnisonDetune"] = std::clamp (unisonDetune * 45.0 + (unisonVoices > 1 ? 8.0 : 0.0), 0.0, 100.0);
            v[oid + "UnisonSpread"] = unisonSpread * 100.0;
        }

        // Guarantee at least Osc 1 is on if both main oscs were muted (e.g. insert-osc / noise patch)
        if (out.oscsMapped == 0)
        {
            v["osc1On"] = 1.0;
            v["osc1Table"] = (double) pickFallbackTable (out.info, {}, 0);
            v["osc1Volume"] = 80.0;
            out.oscsMapped = 1;
        }

        // 3. Auxiliary PM Oscillators 1 & 2 -> WaveSynth Osc 3 & 4 FM
        for (int a = 0; a < 2; ++a)
        {
            const juce::String slot = "AuxiliaryOsc" + juce::String (a + 1) + "Slot";
            const juce::String pref = u1 + "audioPluginSlots/" + slot + "/audioPlugins/PhaseModOscPlugin/";
            const juce::String pmKey = u1 + "audioPluginSlots/MainOsc" + juce::String (a + 1) + "Slot/audioPlugins/Wavetable/PM" + juce::String (a + 1) + "/selectedEntryName";
            const double pmAmt = std::clamp (t.num (pref + "Level/Knob/parameterState/normalizedValue", 0.0), 0.0, 1.0);
            const bool routed = t.str (pmKey) != "-" && t.str (pmKey).isNotEmpty();

            if (routed && pmAmt > 0.02)
            {
                const int targetOsc = a + 3; // Osc 3 / Osc 4
                const juce::String oid = "osc" + juce::String (targetOsc);
                v[oid + "On"] = 1.0;
                v[oid + "Table"] = (double) (int) WaveTables::Table::analog;
                v[oid + "Position"] = t.str (pref + "Mode/selectedEntryName").containsIgnoreCase ("Tri") ? 33.0 : 0.0;
                v[oid + "Volume"] = 0.0; // modulator only
                const int64_t rawPitch = t.integer (pref + "Pitch/TuningKnob/tuningState/rawValue", 0);
                v[oid + "Pitch"] = std::clamp (std::round ((double) rawPitch / 1000.0), -24.0, 24.0);

                const juce::String mainOid = "osc" + juce::String (a + 1);
                if (v[mainOid + "Warp"] == 0.0)
                {
                    v[mainOid + "Warp"] = (double) (int) WaveSynth::Warp::fm;
                    v[mainOid + "FmFrom"] = (double) targetOsc; // choice 1..9 = Osc 1..9
                    v[mainOid + "WarpAmount"] = pmAmt * 80.0;
                }
                ++out.oscsMapped;
            }
        }

        // 4. Noise 1 & 2
        const double noise1 = t.flag (u1 + "audioPluginSlots/Noise1Slot/pluginSlotActive", false)
                            ? std::clamp (t.num (u1 + "audioPluginSlots/Noise1Slot/audioPlugins/NoisePlayer/Level/Knob/parameterState/normalizedValue", 0.0), 0.0, 1.0)
                            : 0.0;
        const double noise2 = t.flag (u1 + "audioPluginSlots/Noise2Slot/pluginSlotActive", false)
                            ? std::clamp (t.num (u1 + "audioPluginSlots/Noise2Slot/audioPlugins/NoisePlayer/Level/Knob/parameterState/normalizedValue", 0.0), 0.0, 1.0)
                            : 0.0;
        v["noiseLevel"] = std::clamp (std::max (noise1, noise2) * 55.0, 0.0, 100.0);

        // 5. FilterSlot -> Filter 1
        const juce::String filterPlugin = t.str (u1 + "audioPluginSlots/FilterSlot/selectedPlugin");
        if (filterPlugin.isNotEmpty() && ! filterPlugin.containsIgnoreCase ("Bypass"))
        {
            const juce::String fPref = u1 + "audioPluginSlots/FilterSlot/audioPlugins/" + filterPlugin + "/";
            const juce::String subType = t.str (fPref + "Type/selectedEntryName", t.str (fPref + "Mode/selectedEntryName"));
            v["filter1Type"] = (double) mapFilterType (filterPlugin, subType);
            v["filter1Cutoff"] = normToCutoffHz (t.num (fPref + "Cutoff/Knob/parameterState/normalizedValue",
                                                        t.num (fPref + "Freq/Knob/parameterState/normalizedValue", 0.72)));
            v["filter1Resonance"] = std::clamp (t.num (fPref + "Res/Knob/parameterState/normalizedValue",
                                                       t.num (fPref + "Resonance/Knob/parameterState/normalizedValue", 0.15)), 0.0, 1.0) * 90.0;
            v["filter1Drive"] = std::clamp (t.num (fPref + "Drive/Knob/parameterState/normalizedValue",
                                                   t.num (fPref + "Feedback/Knob/parameterState/normalizedValue", 0.0)), 0.0, 1.0) * 80.0;
            v["filter1KeyTrack"] = std::clamp (t.num (fPref + "Keytrack/Knob/parameterState/normalizedValue", 0.25), 0.0, 1.0) * 100.0;
        }
        else
        {
            v["filter1Type"] = (double) (int) WaveSynth::FilterType::off;
        }

        // 6. Envelopes (Mod1AmpEnvSlot -> env1; Mod2..9 ModEnvelope/ExciterEnvelope -> env2..4)
        std::map<juce::String, int> modSlotToSource;
        {
            const juce::String aPref = u1 + "modulationPluginSlots/Mod1AmpEnvSlot/modulationPlugins/AmpEnvelope/";
            v["env1Attack"]  = std::max (1.5, normToMs (t.num (aPref + "Attack/Knob/parameterState/normalizedValue", 0.02), 5000.0));
            v["env1Decay"]   = std::max (10.0, normToMs (t.num (aPref + "Decay/Knob/parameterState/normalizedValue", 0.35), 6000.0));
            v["env1Sustain"] = std::clamp (t.num (aPref + "Sustain/Knob/parameterState/normalizedValue", 0.75), 0.0, 1.0) * 100.0;
            v["env1Release"] = std::max (10.0, normToMs (t.num (aPref + "Release/Knob/parameterState/normalizedValue", 0.25), 6000.0));
            v["ampVelocity"] = std::clamp (t.num (aPref + "Velocity/Knob/parameterState/normalizedValue", 0.5), 0.0, 1.0) * 100.0;
            modSlotToSource["Mod1AmpEnvSlot"] = WaveSynth::env1;
        }

        int nextEnv = 2; // env2..env4
        int nextLfo = 1; // lfo1..lfo4

        for (int m = 2; m <= 9; ++m)
        {
            const juce::String slotName = "Mod" + juce::String (m) + "Slot";
            const juce::String plug = t.str (u1 + "modulationPluginSlots/" + slotName + "/selectedPlugin");
            if (plug.isEmpty()) continue;

            if ((plug.containsIgnoreCase ("Envelope")) && nextEnv <= WaveSynth::numEnvelopes)
            {
                const juce::String ePref = u1 + "modulationPluginSlots/" + slotName + "/modulationPlugins/" + plug + "/";
                const juce::String eid = "env" + juce::String (nextEnv);
                v[eid + "Attack"]  = normToMs (t.num (ePref + "Attack/Knob/parameterState/normalizedValue", 0.0), 5000.0);
                v[eid + "Decay"]   = std::max (10.0, normToMs (t.num (ePref + "Decay/Knob/parameterState/normalizedValue", 0.35), 6000.0));
                v[eid + "Sustain"] = std::clamp (t.num (ePref + "Sustain/Knob/parameterState/normalizedValue", 0.0), 0.0, 1.0) * 100.0;
                v[eid + "Release"] = std::max (10.0, normToMs (t.num (ePref + "Release/Knob/parameterState/normalizedValue", 0.25), 6000.0));
                modSlotToSource[slotName] = WaveSynth::env1 + (nextEnv - 1);
                ++nextEnv;
            }
            else if (plug.containsIgnoreCase ("LFO") && nextLfo <= WaveSynth::numLfos)
            {
                const juce::String lPref = u1 + "modulationPluginSlots/" + slotName + "/modulationPlugins/" + plug + "/";
                const juce::String lid = "lfo" + juce::String (nextLfo);
                const double rateNorm = std::clamp (t.num (lPref + "Rate/Knob/parameterState/normalizedValue", 0.35), 0.0, 1.0);
                v[lid + "Rate"] = std::clamp (0.05 * std::pow (400.0, rateNorm), 0.05, 20.0);
                v[lid + "Shape"] = plug.containsIgnoreCase ("Random") ? 5.0 : 0.0; // S&H vs Sine
                modSlotToSource[slotName] = WaveSynth::lfoSource (nextLfo - 1);
                ++nextLfo;
            }
        }

        // 7. Performers -> WaveSynth 16-node MSEG Curve LFOs (when LFO slots remain!)
        const int curveShapeIdx = WaveSynth::lfoShapeNames().indexOf ("Curve");
        for (int p = 0; p < 3 && nextLfo <= WaveSynth::numLfos; ++p)
        {
            const juce::String ptsKey = u1 + "Performers/performers/performer " + juce::String (p) + "/page 0/points";
            const auto it = t.intVecs.find (ptsKey);
            if (it == t.intVecs.end() || it->second.size() < 4) continue;

            const auto& pts = it->second;
            const size_t numPts = pts.size() / 2;
            if (numPts < 2) continue;

            const juce::String lid = "lfo" + juce::String (nextLfo);
            if (curveShapeIdx >= 0)
                v[lid + "Shape"] = (double) curveShapeIdx;
            v[lid + "Sync"] = 1.0;
            v[lid + "Division"] = 7.0; // 1/4 bar / beat

            for (int step = 0; step < WaveSynth::numSteps; ++step)
            {
                const size_t idx = std::min (numPts - 1, (size_t) ((double) step * (double) (numPts - 1) / (double) (WaveSynth::numSteps - 1)));
                const double yNorm = std::clamp ((double) pts[idx * 2 + 1] / 536870912.0, 0.0, 1.0);
                v[lid + "Step" + juce::String (step + 1)] = (yNorm * 2.0 - 1.0) * 100.0;
            }

            modSlotToSource["Performer" + juce::String (p + 1) + "Slot"] = WaveSynth::lfoSource (nextLfo - 1);
            ++nextLfo;
        }

        // 8. Macros 1..4 names
        for (int m = 0; m < WaveSynth::numMacros; ++m)
        {
            const auto mName = t.str ("root/engine/global/macros/macro" + juce::String (m + 1) + "/macroName/value").trim();
            if (mName.isNotEmpty() && mName != "Macro " + juce::String (m + 1))
            {
                out.macroNames[(size_t) m] = mName;
                ++out.macrosMapped;
            }
        }

        // 9. Modulation Matrix -> WaveSynth 24 slots
        int modSlot = 1;
        auto addMod = [&] (int source, const juce::String& destId, double amountPct)
        {
            if (modSlot > WaveSynth::numSlots || source == WaveSynth::none || destId.isEmpty() || std::abs (amountPct) < 1.0)
                return;
            const int dIdx = destinationIndexFor (destId);
            if (dIdx <= 0) return;

            const juce::String ms = "mod" + juce::String (modSlot);
            v[ms + "Source"] = (double) source;
            v[ms + "Destination"] = (double) dIdx;
            v[ms + "Amount"] = std::clamp (amountPct, -100.0, 100.0);
            ++modSlot;
            ++out.modsMapped;
        };

        auto resolveSource = [&] (const juce::String& srcPath) -> int
        {
            if (srcPath.isEmpty()) return WaveSynth::none;
            if (srcPath.contains ("macroMW")) return WaveSynth::modWheel;
            if (srcPath.contains ("macroAT")) return WaveSynth::aftertouch;
            if (srcPath.contains ("macro1") && ! srcPath.contains ("macro10") && ! srcPath.contains ("macro11") && ! srcPath.contains ("macro12")
                && ! srcPath.contains ("macro13") && ! srcPath.contains ("macro14") && ! srcPath.contains ("macro15") && ! srcPath.contains ("macro16"))
                return WaveSynth::macro1;
            if (srcPath.contains ("macro2")) return WaveSynth::macro2;
            if (srcPath.contains ("macro3")) return WaveSynth::macro3;
            if (srcPath.contains ("macro4")) return WaveSynth::macro4;
            if (srcPath.contains ("Velocity")) return WaveSynth::velocity;
            if (srcPath.contains ("Keytrack")) return WaveSynth::key;

            for (const auto& [slotKey, wsSrc] : modSlotToSource)
                if (srcPath.contains (slotKey))
                    return wsSrc;

            return WaveSynth::none;
        };

        for (const auto& [key, connected] : t.bools)
        {
            if (! connected || ! key.endsWith ("/modulationTargetNodeState/isConnected"))
                continue;
            const auto base = key.dropLastCharacters (11); // drop "isConnected"
            if (t.flag (base + "isMuted", false))
                continue;

            const auto srcPath = t.str (base + "modulationSource");
            const int src = resolveSource (srcPath);
            const auto destId = mapTargetPathToDestId (key);
            if (src == WaveSynth::none || destId.isEmpty())
                continue;

            // Modulation amount lives alongside modulationTargetNodeState in modulationAmountState/normalizedValue
            const auto targetBase = key.upToLastOccurrenceOf ("/modulationTargetNodeState/isConnected", false, false);
            const double normAmt = t.num (targetBase + "/modulationAmountState/normalizedValue", 0.5);
            const double pct = (normAmt - 0.5) * 200.0;
            addMod (src, destId, pct);
        }

        // Also wire explicit 1-to-1 macro assignments (macro1..4)
        for (int m = 0; m < WaveSynth::numMacros; ++m)
        {
            const auto path = t.str ("root/engine/global/macros/macro" + juce::String (m + 1) + "/macroAssignment/oneToOneMappingPath");
            if (path.isNotEmpty())
            {
                const auto destId = mapTargetPathToDestId (path + "/");
                if (destId.isNotEmpty())
                    addMod (WaveSynth::macro1 + m, destId, 50.0);
            }
        }

        // 10. Insert FX & Unit FX -> WaveSynth built-in Drive / Chorus / Phaser / Delay / Reverb
        for (const auto& slot : { "InsertFX1Slot", "InsertFX2Slot", "InsertFX3Slot", "UnitFX1Slot", "UnitFX2Slot", "UnitFX3Slot" })
        {
            const juce::String slotPref = u1 + "audioPluginSlots/" + slot + "/";
            const juce::String plug = t.str (slotPref + "selectedPlugin");
            if (plug.isEmpty() || plug.containsIgnoreCase ("Bypass")) continue;

            const juce::String fxPref = slotPref + "audioPlugins/" + plug + "/";
            const double dw = std::clamp (t.num (fxPref + "DW/Knob/parameterState/normalizedValue",
                                                 t.num (fxPref + "Mix/Knob/parameterState/normalizedValue", 0.5)), 0.0, 1.0);

            if ((plug.containsIgnoreCase ("Dist") || plug.containsIgnoreCase ("Folder") || plug.containsIgnoreCase ("BitCrusher") || plug.containsIgnoreCase ("NonLinear")) && v["fxDriveOn"] == 0.0)
            {
                const double drv = std::clamp (t.num (fxPref + "Drive/Knob/parameterState/normalizedValue", 0.4), 0.0, 1.0);
                if (drv > 0.02 || dw > 0.02)
                {
                    v["fxDriveOn"] = 1.0;
                    v["fxDriveType"] = plug.containsIgnoreCase ("BitCrusher") ? 5.0 : (plug.containsIgnoreCase ("Folder") ? 4.0 : 0.0);
                    v["fxDriveAmount"] = std::clamp (drv * 85.0, 10.0, 100.0);
                    v["fxDriveMix"] = std::clamp (dw * 100.0, 20.0, 100.0);
                    ++out.fxMapped;
                }
            }
            else if ((plug.containsIgnoreCase ("Chorus") || plug.containsIgnoreCase ("Dimension") || plug.containsIgnoreCase ("Flanger")) && dw > 0.02 && v["fxChorusOn"] == 0.0)
            {
                v["fxChorusOn"] = 1.0;
                v["fxChorusMix"] = std::clamp (dw * 100.0, 10.0, 100.0);
                v["fxChorusDepth"] = std::clamp (t.num (fxPref + "Depth/Knob/parameterState/normalizedValue", 0.45) * 100.0, 10.0, 100.0);
                ++out.fxMapped;
            }
            else if (plug.containsIgnoreCase ("Phaser") && dw > 0.02 && v["fxPhaserOn"] == 0.0)
            {
                v["fxPhaserOn"] = 1.0;
                v["fxPhaserMix"] = std::clamp (dw * 100.0, 10.0, 100.0);
                v["fxPhaserFeedback"] = std::clamp (t.num (fxPref + "Feed/Knob/parameterState/normalizedValue", 0.45) * 100.0, 0.0, 95.0);
                ++out.fxMapped;
            }
            else if (plug.containsIgnoreCase ("Delay") && dw > 0.02 && v["fxDelayOn"] == 0.0)
            {
                v["fxDelayOn"] = 1.0;
                v["fxDelayMix"] = std::clamp (dw * 100.0, 5.0, 80.0);
                v["fxDelayFeedback"] = std::clamp (t.num (fxPref + "Feed/Knob/parameterState/normalizedValue", 0.35) * 85.0, 0.0, 85.0);
                ++out.fxMapped;
            }
            else if (plug.containsIgnoreCase ("Reverb") && dw > 0.02 && v["fxReverbOn"] == 0.0)
            {
                v["fxReverbOn"] = 1.0;
                v["fxReverbMix"] = std::clamp (dw * 100.0, 5.0, 80.0);
                v["fxReverbSize"] = std::clamp (t.num (fxPref + "Size/Knob/parameterState/normalizedValue", 0.55) * 100.0, 10.0, 100.0);
                v["fxReverbDecay"] = std::clamp (0.4 + t.num (fxPref + "Feed/Knob/parameterState/normalizedValue", 0.4) * 5.0, 0.2, 12.0);
                ++out.fxMapped;
            }
        }
    }
}

//==============================================================================
juce::File defaultLocalWavetablesFolder()
{
   #if JUCE_MAC
    const juce::File dir ("/Library/Application Support/Native Instruments/Massive X/Wavetables");
   #elif JUCE_WINDOWS
    const juce::File dir = juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory)
                               .getParentDirectory().getChildFile ("Program Files/Common Files/Native Instruments/Massive X/Wavetables");
   #else
    const juce::File dir;
   #endif
    return dir.isDirectory() ? dir : juce::File();
}

bool isMassiveXPreset (const juce::File& file)
{
    if (! file.existsAsFile() || file.getSize() < 48)
        return false;

    juce::FileInputStream in (file);
    if (! in.openedOk())
        return false;

    uint8 buf[4096];
    const int n = in.read (buf, (int) sizeof (buf));
    if (n < 24 || std::memcmp (buf, "RIFF", 4) != 0 || std::memcmp (buf + 8, "NIKS", 4) != 0)
        return false;

    size_t pos = 12;
    while (pos + 8 <= (size_t) n)
    {
        const uint32 sz = (uint32) buf[pos + 4] | ((uint32) buf[pos + 5] << 8)
                        | ((uint32) buf[pos + 6] << 16) | ((uint32) buf[pos + 7] << 24);
        if (std::memcmp (buf + pos, "PLID", 4) == 0 && pos + 8 + sz <= (size_t) n)
            return parsePlidIsMassiveX (buf + pos + 8, sz);
        pos += 8 + sz + (sz & 1u);
    }
    return false;
}

ImportResult importMemory (const void* data, size_t size, const juce::File& localWavetablesFolder)
{
    ImportResult out;
    if (data == nullptr || size < 32)
    {
        out.error = "This file is too small to be a Massive X (.nksf) preset.";
        return out;
    }

    const auto* b = (const uint8*) data;
    if (std::memcmp (b, "RIFF", 4) != 0 || std::memcmp (b + 8, "NIKS", 4) != 0)
    {
        out.error = "This is not an NI NKSF (RIFF/NIKS) preset file.";
        return out;
    }

    const uint8* nisiPtr = nullptr; size_t nisiLen = 0;
    const uint8* plidPtr = nullptr; size_t plidLen = 0;
    const uint8* pchkPtr = nullptr; size_t pchkLen = 0;

    size_t pos = 12;
    while (pos + 8 <= size)
    {
        const uint32 sz = (uint32) b[pos + 4] | ((uint32) b[pos + 5] << 8)
                        | ((uint32) b[pos + 6] << 16) | ((uint32) b[pos + 7] << 24);
        if (sz > size - (pos + 8))
        {
            out.error = "The NKSF preset container is truncated or damaged.";
            return out;
        }
        if (std::memcmp (b + pos, "NISI", 4) == 0)      { nisiPtr = b + pos + 8; nisiLen = sz; }
        else if (std::memcmp (b + pos, "PLID", 4) == 0) { plidPtr = b + pos + 8; plidLen = sz; }
        else if (std::memcmp (b + pos, "PCHK", 4) == 0) { pchkPtr = b + pos + 8; pchkLen = sz; }
        pos += 8 + (size_t) sz + (sz & 1u);
    }

    if (plidPtr != nullptr && ! parsePlidIsMassiveX (plidPtr, plidLen))
    {
        out.error = "This .nksf preset belongs to another instrument, not Massive X.";
        return out;
    }

    if (pchkPtr == nullptr || pchkLen <= 24)
    {
        out.error = "This .nksf preset has no Massive X patch payload (PCHK).";
        return out;
    }

    if (nisiPtr != nullptr)
        parseNisi (nisiPtr, nisiLen, out.info);

    // Locate zlib header (0x78 0x9c / 0x78 0xda / 0x78 0x01) at +20 (or scan first 32 bytes)
    size_t zlibOff = 20;
    if (pchkPtr[zlibOff] != 0x78)
    {
        bool found = false;
        for (size_t off = 4; off + 6 < std::min<size_t> (pchkLen, 48); ++off)
        {
            if (pchkPtr[off] == 0x78 && (pchkPtr[off + 1] == 0x9c || pchkPtr[off + 1] == 0xda || pchkPtr[off + 1] == 0x01))
            {
                zlibOff = off;
                found = true;
                break;
            }
        }
        if (! found)
        {
            out.error = "Could not locate uncompressed Massive X patch data inside PCHK.";
            return out;
        }
    }

    juce::MemoryBlock dec;
    if (! inflateZlib (pchkPtr + zlibOff, pchkLen - zlibOff, dec))
    {
        out.error = "Could not decompress the Massive X patch payload.";
        return out;
    }

    MxTree tree;
    if (! parsePchkTree ((const uint8*) dec.getData(), dec.getSize(), tree))
    {
        out.error = "Could not parse the Massive X parameter tree.";
        return out;
    }

    if (out.info.name.isEmpty())
        out.info.name = tree.str ("meta/presetName", "Imported Massive X");

    const auto wtDir = localWavetablesFolder.isDirectory() ? localWavetablesFolder : defaultLocalWavetablesFolder();
    translateTree (tree, wtDir, out);

    out.ok = true;
    out.summary = out.info.name
        + (out.info.author.isNotEmpty() ? " by " + out.info.author : "")
        + " — " + juce::String (out.oscsMapped) + " osc, "
        + juce::String (out.modsMapped) + " mod routes, "
        + juce::String (out.macrosMapped) + " macros, "
        + juce::String (out.fxMapped) + " FX"
        + (out.localWavetablesLoaded > 0
               ? " (" + juce::String (out.localWavetablesLoaded) + " local .mxwt wavetable" + (out.localWavetablesLoaded == 1 ? "" : "s") + " loaded in place)"
               : " (mapped to Torqwave built-in wavetables)");
    return out;
}

ImportResult importFile (const juce::File& nksfFile, const juce::File& localWavetablesFolder)
{
    ImportResult out;
    if (! nksfFile.existsAsFile())
    {
        out.error = "Preset file not found: " + nksfFile.getFullPathName();
        return out;
    }
    juce::MemoryBlock raw;
    if (! nksfFile.loadFileAsData (raw))
    {
        out.error = "Could not read preset file: " + nksfFile.getFullPathName();
        return out;
    }
    out = importMemory (raw.getData(), raw.getSize(), localWavetablesFolder);
    if (out.ok && out.info.name.isEmpty())
        out.info.name = nksfFile.getFileNameWithoutExtension();
    return out;
}

bool applyToSynth (BuiltInEffects::BasicSynthProcessor& synth, const ImportResult& imported)
{
    if (! imported.ok)
        return false;

    for (int o = 0; o < WaveSynth::numOscillators; ++o)
        synth.setSampledTable (o, imported.sampledTables[(size_t) o]);

    synth.setPresetValues (imported.values);
    synth.setPatchName (imported.info.name.isNotEmpty() ? imported.info.name : "Massive X Import");

    for (int m = 0; m < WaveSynth::numMacros; ++m)
        synth.setMacroName (m, imported.macroNames[(size_t) m]);

    return true;
}
}
