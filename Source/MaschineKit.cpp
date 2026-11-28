#include "MaschineKit.h"

namespace MaschineKit
{
namespace
{
    using juce::uint8;
    using juce::uint16;
    using juce::uint32;
    using juce::uint64;

    constexpr char itemTag[] = "hsin", dataDomain[] = "DSIN";
    constexpr uint32 endFrame = 1, metaFrame = 108, archiveFrame = 109;
    constexpr int itemHeaderBytes = 40, frameHeaderBytes = 20, mostDepth = 16;

    const juce::String notAMaschineFile = "This is not a Maschine 2/3 kit (.mxgrp) or sound (.mxsnd) file.";
    const juce::String damagedFile = "The Maschine kit file is damaged or cut short.";

    //==============================================================================
    struct In
    {
        const uint8* data = nullptr;
        size_t size = 0, at = 0;
        bool ok = true;

        size_t left() const noexcept                { return ok ? size - at : 0; }

        bool need (size_t n) noexcept
        {
            if (! ok || n > size - at)
                ok = false;

            return ok;
        }

        void skip (size_t n) noexcept
        {
            if (need (n))
                at += n;
        }

        uint8 u8() noexcept                         { return need (1) ? data[at++] : (uint8) 0; }

        uint16 u16() noexcept
        {
            if (! need (2))
                return 0;

            const auto v = (uint16) (data[at] | (data[at + 1] << 8));
            at += 2;
            return v;
        }

        uint32 u32() noexcept
        {
            if (! need (4))
                return 0;

            const auto v = (uint32) data[at] | ((uint32) data[at + 1] << 8)
                         | ((uint32) data[at + 2] << 16) | ((uint32) data[at + 3] << 24);
            at += 4;
            return v;
        }

        uint64 u64() noexcept
        {
            const auto low = (uint64) u32();
            return low | ((uint64) u32() << 32);
        }

        In take (uint64 n) noexcept
        {
            if (n > (uint64) (size - at) || ! ok)
            {
                ok = false;
                return {};
            }

            In part { data + at, (size_t) n, 0, true };
            at += (size_t) n;
            return part;
        }

        bool tagged (const char* tag) noexcept
        {
            if (! need (4))
                return false;

            const auto same = std::memcmp (data + at, tag, 4) == 0;
            at += 4;
            return same;
        }
    };

    bool findInFrames (In frame, const char* domain, uint32 id, In& found, int depth)
    {
        if (depth > mostDepth)
            return false;

        const auto size = frame.u64();

        if (! frame.ok || size != frame.size)
            return false;

        const auto isData = std::memcmp (frame.data + frame.at, dataDomain, 4) == 0;
        const auto isWanted = frame.tagged (domain);
        const auto itsId = frame.u32();
        frame.skip (4);

        if (! frame.ok)
            return false;

        if (isData && itsId == endFrame)
            return false;

        In inner = frame;
        const auto innerSize = inner.u64();

        if (! inner.ok || innerSize < (uint64) frameHeaderBytes)
            return false;

        const auto inside = frame.take (innerSize);

        if (! frame.ok)
            return false;

        if (isWanted && itsId == id)
        {
            found = frame.take (frame.left());
            return true;
        }

        return findInFrames (inside, domain, id, found, depth + 1);
    }

    bool findInItem (In& item, const char* domain, uint32 id, In& found, int depth)
    {
        if (depth > mostDepth)
            return false;

        In start = item;
        const auto size = start.u64();

        if (! start.ok || size < (uint64) itemHeaderBytes)
            return false;

        auto body = item.take (size);

        if (! item.ok)
            return false;

        body.skip (12);

        if (! body.tagged (itemTag))
            return false;

        body.skip (24);

        In frames = body;
        const auto framesSize = frames.u64();

        if (! frames.ok || framesSize < (uint64) frameHeaderBytes)
            return false;

        const auto frame = body.take (framesSize);

        if (! body.ok)
            return false;

        if (findInFrames (frame, domain, id, found, 0))
            return true;

        body.skip (4);
        const auto children = body.u32();

        for (uint32 i = 0; i < children && body.ok && body.left() > 0; ++i)
        {
            body.skip (12);

            if (! body.ok)
                return false;

            if (findInItem (body, domain, id, found, depth + 1))
                return true;
        }

        return false;
    }

    //==============================================================================
    // Boost portable_binary_iarchive primitive helpers

    bool readPint (const uint8* d, size_t len, size_t& pos, int64_t& out) noexcept
    {
        if (pos >= len)
            return false;

        const auto sz = (int8_t) d[pos++];

        if (sz == 0)
        {
            out = 0;
            return true;
        }

        const bool neg = sz < 0;
        const size_t n = (size_t) (neg ? -sz : sz);

        if (n > 8 || pos + n > len)
            return false;

        uint64 v = 0;

        for (size_t i = 0; i < n; ++i)
            v |= ((uint64) d[pos++]) << (8 * i);

        out = neg ? -(int64_t) v : (int64_t) v;
        return true;
    }

    bool readPString (const uint8* d, size_t len, size_t& pos, juce::String& out, size_t maxChars = (size_t) mostNameLength)
    {
        int64_t slen = 0;

        if (! readPint (d, len, pos, slen) || slen < 0 || (size_t) slen > maxChars || pos + (size_t) slen > len)
            return false;

        out = juce::String::fromUTF8 ((const char*) (d + pos), (int) slen);
        pos += (size_t) slen;
        return true;
    }

    float readF32 (const uint8* d, size_t pos) noexcept
    {
        float v = 0.0f;
        std::memcpy (&v, d + pos, sizeof (v));
        return std::isfinite (v) ? v : 0.0f;
    }

    bool isPrintableString (const uint8* d, size_t n) noexcept
    {
        if (n == 0)
            return false;

        bool hasNonSpace = false;

        for (size_t i = 0; i < n; ++i)
        {
            if (d[i] < 0x20 || d[i] == 0x7f)
                return false;
            if (d[i] != ' ')
                hasNonSpace = true;
        }

        return hasNonSpace;
    }

    //==============================================================================
    ModuleKind kindFromClassName (const juce::String& cls)
    {
        if (cls == "Sampler")                return ModuleKind::sampler;
        if (cls == "AudioModule" || cls == "Audio") return ModuleKind::audioModule;
        if (cls == "Kick")                   return ModuleKind::kick;
        if (cls == "Snare")                  return ModuleKind::snare;
        if (cls == "Hihat")                  return ModuleKind::hihat;
        if (cls == "Tom")                    return ModuleKind::tom;
        if (cls == "Perc")                   return ModuleKind::perc;
        if (cls == "Cymbal")                 return ModuleKind::cymbal;
        if (cls == "BassSynth" || cls == "Bass Synth") return ModuleKind::bassSynth;
        if (cls == "EQ")                     return ModuleKind::eq;
        if (cls == "Filter")                 return ModuleKind::filter;
        if (cls == "Saturator")              return ModuleKind::saturator;
        if (cls == "MullhollandDrive" || cls == "Distortion") return ModuleKind::distortion;
        if (cls == "Compressor")             return ModuleKind::compressor;
        if (cls == "Maximizer")              return ModuleKind::maximizer;
        if (cls == "Limiter")                return ModuleKind::limiter;
        if (cls == "TransientDesigner" || cls == "Transient Master") return ModuleKind::transientDesigner;
        if (cls == "LoFi" || cls == "Lofi")  return ModuleKind::lofi;
        if (cls == "BeatDelay" || cls == "Beat Delay") return ModuleKind::beatDelay;
        if (cls == "Reflex")                 return ModuleKind::reflex;
        if (cls == "GrainDelay" || cls == "Grain Delay") return ModuleKind::grainDelay;
        if (cls == "Metaverb")               return ModuleKind::metaverb;
        if (cls == "Reverb" || cls == "Reverb (Legacy)") return ModuleKind::reverb;
        if (cls == "PlateReverb")            return ModuleKind::plateReverb;
        if (cls == "Iceverb" || cls == "Ice") return ModuleKind::iceverb;
        if (cls == "CabinetEmulation" || cls == "Cabinet") return ModuleKind::cabinet;
        if (cls == "Gate")                   return ModuleKind::gate;
        if (cls == "Chorus")                 return ModuleKind::chorus;
        if (cls == "Flanger")                return ModuleKind::flanger;
        if (cls == "Phaser")                 return ModuleKind::phaser;
        if (cls == "FreqShifter" || cls == "Freq Shifter") return ModuleKind::freqShifter;
        if (cls == "FM")                     return ModuleKind::fm;
        if (cls == "Resochord")              return ModuleKind::resochord;
        if (cls == "GrainStretch")           return ModuleKind::grainStretch;
        if (cls == "PluginHost")             return ModuleKind::pluginHost;
        return ModuleKind::unknown;
    }

    bool isEventClass (const juce::String& cls)
    {
        return cls == "NoteEvent" || cls == "FloatModulationEvent" || cls == "MidiEvent" || cls == "AudioEvent";
    }

    //==============================================================================
    /** Scans sequential Boost-tracked Parameter records (each headed by 3..6 consecutive object_ids
        followed by 0x00) inside [start, end) and extracts their float or int primary values. */
    struct RawParam
    {
        bool isFloat = false;
        float f = 0.0f;
        int i = 0;
        size_t offset = 0;
    };

    std::vector<RawParam> scanParams (const uint8* d, size_t start, size_t end, int maxOut = mostParams)
    {
        std::vector<RawParam> out;
        size_t p = start;
        int64_t expectedOid = -1;

        while (p + 6 < end && (int) out.size() < maxOut)
        {
            const auto sz = (int8_t) d[p];

            if (sz >= 1 && sz <= 3 && p + 1 + (size_t) sz <= end)
            {
                int64_t val = 0;

                for (int b = 0; b < sz; ++b)
                    val |= ((int64_t) d[p + 1 + (size_t) b]) << (8 * b);

                if (expectedOid < 0 || val == expectedOid)
                {
                    size_t cur = p;
                    int64_t curOid = val;
                    int cnt = 0;

                    while (cur + 2 < end)
                    {
                        const auto s2 = (int8_t) d[cur];

                        if (s2 >= 1 && s2 <= 3 && cur + 1 + (size_t) s2 <= end)
                        {
                            int64_t v2 = 0;

                            for (int b = 0; b < s2; ++b)
                                v2 |= ((int64_t) d[cur + 1 + (size_t) b]) << (8 * b);

                            if (v2 == curOid)
                            {
                                cur += 1 + (size_t) s2;
                                ++curOid;
                                ++cnt;
                                continue;
                            }
                        }

                        break;
                    }

                    if (cnt >= 3 && cur < end && d[cur] == 0)
                    {
                        const size_t vo = cur + 1;
                        bool matchedFloat = false;

                        if (vo + 17 <= end && d[vo + 16] == 0)
                        {
                            const float f0 = readF32 (d, vo);
                            const float f1 = readF32 (d, vo + 4);
                            const float f2 = readF32 (d, vo + 8);
                            const float f3 = readF32 (d, vo + 12);

                            if (juce::exactlyEqual (f0, f1) && std::abs (f0) < 1.0e6f && std::abs (f2) < 1.0e6f && std::abs (f3) < 1.0e6f)
                            {
                                const size_t np = vo + 17;

                                if (np + 2 < end)
                                {
                                    const auto ns = (int8_t) d[np];

                                    if (ns >= 1 && ns <= 3 && np + 1 + (size_t) ns <= end)
                                    {
                                        int64_t nv = 0;

                                        for (int b = 0; b < ns; ++b)
                                            nv |= ((int64_t) d[np + 1 + (size_t) b]) << (8 * b);

                                        if (nv == curOid)
                                            matchedFloat = true;
                                    }
                                }
                                else if (np >= end - 4)
                                {
                                    matchedFloat = true;
                                }
                            }

                            if (matchedFloat)
                            {
                                out.push_back ({ true, f0, 0, p });
                                expectedOid = curOid;
                                p = vo + 17;
                                continue;
                            }
                        }

                        // Double parameter (4 doubles + 0x00, e.g. Zone stretch/tune)
                        if (vo + 33 <= end && d[vo + 32] == 0)
                        {
                            double d0 = 0.0, d1 = 0.0;
                            std::memcpy (&d0, d + vo, sizeof (d0));
                            std::memcpy (&d1, d + vo + 8, sizeof (d1));

                            if (std::isfinite (d0) && juce::exactlyEqual (d0, d1))
                            {
                                const size_t np = vo + 33;

                                if (np + 2 < end)
                                {
                                    const auto ns = (int8_t) d[np];

                                    if (ns >= 1 && ns <= 3 && np + 1 + (size_t) ns <= end)
                                    {
                                        int64_t nv = 0;

                                        for (int b = 0; b < ns; ++b)
                                            nv |= ((int64_t) d[np + 1 + (size_t) b]) << (8 * b);

                                        if (nv == curOid)
                                        {
                                            out.push_back ({ true, (float) d0, 0, p });
                                            expectedOid = curOid;
                                            p = np;
                                            continue;
                                        }
                                    }
                                }
                            }
                        }

                        // Integer / Bool / Enum parameter
                        size_t pos = vo;
                        int firstInt = 0;
                        bool gotFirst = false;

                        for (int k = 0; k < 6 && pos < end; ++k)
                        {
                            const auto ns = (int8_t) d[pos];

                            if (ns >= 1 && ns <= 3 && pos + 1 + (size_t) ns <= end)
                            {
                                int64_t nv = 0;

                                for (int b = 0; b < ns; ++b)
                                    nv |= ((int64_t) d[pos + 1 + (size_t) b]) << (8 * b);

                                if (nv == curOid)
                                    break;
                            }

                            int64_t iv = 0;

                            if (! readPint (d, end, pos, iv))
                                break;

                            if (! gotFirst)
                            {
                                firstInt = (int) juce::jlimit<int64_t> (INT_MIN, INT_MAX, iv);
                                gotFirst = true;
                            }
                        }

                        out.push_back ({ false, 0.0f, firstInt, p });
                        expectedOid = curOid;
                        p = pos;
                        continue;
                    }
                }
            }

            ++p;
        }

        return out;
    }

    //==============================================================================
    /** Checks if a string looks like an audio sample relative path. */
    bool looksLikeAudioPath (const juce::String& s)
    {
        if (s.length() < 5 || s.length() > 512)
            return false;

        const auto lower = s.toLowerCase();
        return lower.endsWith (".wav") || lower.endsWith (".wave") || lower.endsWith (".aif")
            || lower.endsWith (".aiff") || lower.endsWith (".flac") || lower.endsWith (".ogg")
            || lower.endsWith (".mp3") || lower.endsWith (".m4a");
    }

    /** Extracts Sampler / AudioModule zones inside [modStart, modEnd) along with the byte offset
        immediately after the last zone's metadata so Sampler engine params can be scanned next. */
    void parseZones (const uint8* d, size_t modStart, size_t modEnd, std::vector<Zone>& zones, size_t& afterLastZonePath)
    {
        afterLastZonePath = modStart;
        size_t p = modStart;

        while (p + 8 < modEnd && (int) zones.size() < mostZones)
        {
            // Portable string: 1-byte or 2-byte positive length followed by UTF-8 path ending in audio extension
            const auto sz = (int8_t) d[p];

            if ((sz == 1 || sz == 2) && p + 1 + (size_t) sz < modEnd)
            {
                size_t q = p;
                juce::String rel;

                if (readPString (d, modEnd, q, rel, 512) && looksLikeAudioPath (rel) && isPrintableString (d + p + 1 + (size_t) sz, (size_t) rel.getNumBytesAsUTF8()))
                {
                    Zone z;
                    z.relativePath = rel.replaceCharacter ('\\', '/');

                    // Optional 0x00 0x00 separator (seen on Zone 0) before basePath + bankName
                    size_t metaPos = q;

                    if (metaPos + 2 < modEnd && d[metaPos] == 0 && d[metaPos + 1] == 0)
                        metaPos += 2;

                    juce::String base, bank;

                    if (readPString (d, modEnd, metaPos, base, 512) && base.isNotEmpty())
                    {
                        z.basePath = base;
                        size_t bankPos = metaPos;

                        if (readPString (d, modEnd, bankPos, bank, 256))
                        {
                            z.bankName = bank;
                            metaPos = bankPos;
                        }
                    }

                    // Scan the 21 Zone parameters immediately following the path strings
                    const size_t zoneEnd = std::min (modEnd, metaPos + 640);
                    const auto zp = scanParams (d, metaPos, zoneEnd, 22);

                    if (zp.size() >= 16)
                    {
                        if (! zp[0].isFloat)  z.startFrame = juce::jmax (0, zp[0].i);
                        if (! zp[1].isFloat)  z.endFrame = juce::jmax (0, zp[1].i);
                        if (! zp[8].isFloat && zp[8].i >= 0 && zp[8].i <= 127)   z.rootKey = zp[8].i;
                        if (! zp[9].isFloat && zp[9].i >= 0 && zp[9].i <= 127)   z.lowKey = zp[9].i;
                        if (! zp[10].isFloat && zp[10].i >= 0 && zp[10].i <= 127) z.highKey = zp[10].i;
                        if (! zp[11].isFloat && zp[11].i >= 0 && zp[11].i <= 127) z.lowVelocity = zp[11].i;
                        if (! zp[12].isFloat && zp[12].i >= 0 && zp[12].i <= 127) z.highVelocity = zp[12].i;
                        if (zp[13].isFloat && zp[13].f >= 0.0f && zp[13].f <= 2.0f) z.gain = zp[13].f;
                        if (zp[14].isFloat && std::abs (zp[14].f) <= 1.01f) z.pan = zp[14].f;
                        if (zp[15].isFloat && std::abs (zp[15].f) <= 48.0f) z.tune = zp[15].f;
                        if (zp.size() >= 20 && ! zp[19].isFloat && zp[19].i > 0) z.totalFrames = zp[19].i;
                    }

                    if (z.totalFrames <= 0)
                        z.totalFrames = z.endFrame > 0 ? z.endFrame + 1 : 0;

                    zones.push_back (std::move (z));
                    afterLastZonePath = metaPos;
                    p = metaPos;
                    continue;
                }
            }

            ++p;
        }
    }

    /** Extracts a UTF-16LE string prefixed by a u32 character count. */
    bool readUtf16 (const uint8* d, size_t len, size_t pos, juce::String& out, size_t maxChars = 64)
    {
        if (pos + 4 > len)
            return false;

        const uint32 chars = (uint32) d[pos] | ((uint32) d[pos + 1] << 8)
                           | ((uint32) d[pos + 2] << 16) | ((uint32) d[pos + 3] << 24);

        if (chars == 0 || chars > maxChars || pos + 4 + (size_t) chars * 2 > len)
            return false;

        std::vector<juce::juce_wchar> w;
        w.reserve ((size_t) chars + 1);

        for (uint32 i = 0; i < chars; ++i)
        {
            const auto c = (juce::juce_wchar) (d[pos + 4 + i * 2] | (d[pos + 4 + i * 2 + 1] << 8));

            if (c < 0x20 || c >= 0xd800)
                return false;

            w.push_back (c);
        }

        w.push_back (0);
        out = juce::String (juce::CharPointer_UTF32 (w.data()));
        return true;
    }

    //==============================================================================
    /** Parses every module inside a Sound or Group slice [sliceStart, sliceEnd). */
    void parseModulesInSlice (const uint8* d, size_t sliceStart, size_t sliceEnd,
                              const std::map<int, juce::String>& classMap,
                              std::vector<Module>& modules)
    {
        struct FoundMod
        {
            size_t bodyStart = 0;
            size_t metaPos = 0;
            juce::String typeName;
            ModuleKind kind = ModuleKind::unknown;
        };

        std::vector<FoundMod> found;

        // Pass 1: Locate every module by its Boost class registration ("NI::MASCHINE::DATA::<Class>")
        // OR by its subsequent Boost class_id polymorphic pointer (\x00\x00\x00\x01 <cid> <oid>)
        // AND corroborate with the trailing UTF-16 "Maschine 2.0 Library" module tag.
        static constexpr uint8 libTag[] = {
            'M',0,'a',0,'s',0,'c',0,'h',0,'i',0,'n',0,'e',0,' ',0,'2',0,'.',0,'0',0,' ',0,'L',0,'i',0,'b',0,'r',0,'a',0,'r',0,'y',0
        };
        static constexpr size_t libTagLen = sizeof (libTag);

        // First collect all UTF-16 slot boundary tags ("Maschine 2.0 Library" + module display name)
        struct SlotTag
        {
            size_t pos = 0;
            juce::String tag;
        };

        std::vector<SlotTag> tags;

        for (size_t p = sliceStart; p + libTagLen + 8 < sliceEnd; ++p)
        {
            if (d[p] == 'M' && d[p + 1] == 0 && std::memcmp (d + p, libTag, libTagLen) == 0)
            {
                juce::String tag;

                if (readUtf16 (d, sliceEnd, p + libTagLen, tag, 48))
                {
                    tags.push_back ({ p, tag });
                    p += libTagLen + 4 + (size_t) tag.length() * 2;
                }
            }
        }

        // Also find every explicit "NI::MASCHINE::DATA::" string and every class_id polymorphic slot header
        struct ClassHit
        {
            size_t pos = 0;
            juce::String cls;
        };

        std::vector<ClassHit> hits;
        const char prefix[] = "NI::MASCHINE::DATA::";
        constexpr size_t prefixLen = sizeof (prefix) - 1;

        for (size_t p = sliceStart; p + prefixLen + 2 < sliceEnd; ++p)
        {
            if (d[p] == 'N' && d[p + 1] == 'I' && std::memcmp (d + p, prefix, prefixLen) == 0)
            {
                size_t q = p + prefixLen;

                while (q < sliceEnd && ((d[q] >= 'A' && d[q] <= 'Z') || (d[q] >= 'a' && d[q] <= 'z') || (d[q] >= '0' && d[q] <= '9') || d[q] == '_'))
                    ++q;

                const juce::String cls ((const char*) (d + p + prefixLen), q - (p + prefixLen));

                if (cls.isNotEmpty() && ! isEventClass (cls))
                    hits.push_back ({ q, cls });

                p = q;
            }
        }

        // Also detect repeated (already-registered) polymorphic modules inside Slots:
        // Slot header pattern right before polymorphic pointer: 00 00 00 00 01 <cid> <oid> (or 01 01 01 01 00 00 00 00 01 <cid>)
        for (size_t p = sliceStart; p + 14 < sliceEnd; ++p)
        {
            // Look for: 01 01 01 01 00 00 00 (active slot) or 00 00 00 00 00 00 00 (bypassed slot) followed by class_id
            if (d[p] == 0 && d[p + 1] == 0 && d[p + 2] == 0)
            {
                size_t q = p + 3;

                // Skip optional 0x00 and optional Boost 0x01 0x01 pointer flags
                if (q < sliceEnd && d[q] == 0) ++q;

                // If next is 0x01 0x01 0x01 <cid> followed by "NI::", already captured above
                if (q + 4 < sliceEnd && d[q] == 1 && d[q + 1] == 1 && d[q + 2] == 1)
                    continue;

                // Repeated polymorphic pointer: 0x01 <cid> followed by tracked object_id (0x02 xx yy)
                if (q + 5 < sliceEnd && d[q] == 1)
                {
                    const int cid = (int) d[q + 1];
                    const auto it = classMap.find (cid);

                    if (it != classMap.end() && ! isEventClass (it->second))
                    {
                        const size_t oidPos = q + 2;
                        const auto osz = (int8_t) d[oidPos];

                        if (osz >= 1 && osz <= 3 && oidPos + 1 + (size_t) osz < sliceEnd)
                        {
                            // Verify it sits just after a Slot active/bypass bool param (4 or 5 bytes back)
                            if (p >= sliceStart + 4 && ((d[p - 4] == 1 && d[p - 3] == 1 && d[p - 2] == 1 && d[p - 1] == 1)
                                                        || (d[p - 4] == 0 && d[p - 3] == 0 && d[p - 2] == 0 && d[p - 1] == 0)))
                            {
                                hits.push_back ({ oidPos, it->second });
                                p = oidPos + 1 + (size_t) osz;
                            }
                        }
                    }
                }
            }
        }

        std::sort (hits.begin(), hits.end(), [] (const ClassHit& a, const ClassHit& b) { return a.pos < b.pos; });

        // Deduplicate hits within 32 bytes of each other
        std::vector<ClassHit> uniqueHits;

        for (auto& h : hits)
            if (uniqueHits.empty() || h.pos > uniqueHits.back().pos + 32)
                uniqueHits.push_back (h);

        // Combine SlotTags (which guarantee we never miss ANY slot even if class_id encoding varies)
        // with ClassHits (which tell us exact DrumSynth type: Kick vs Snare vs Hihat vs Tom vs Perc vs Cymbal).
        if (! tags.empty())
        {
            size_t prevEnd = sliceStart;

            for (size_t i = 0; i < tags.size() && (int) found.size() < mostModules; ++i)
            {
                const size_t tagPos = tags[i].pos;
                juce::String typeName = tags[i].tag;
                size_t bodyStart = prevEnd;

                // Look for a matching ClassHit in [prevEnd, tagPos)
                for (auto& h : uniqueHits)
                {
                    if (h.pos >= prevEnd && h.pos < tagPos)
                    {
                        bodyStart = h.pos;

                        if (typeName == "Drumsynth" || kindFromClassName (h.cls) != ModuleKind::unknown)
                            typeName = h.cls;

                        break;
                    }
                }

                // If tag is "Drumsynth" and no explicit class string was inside this slot, infer from class_id or pad name later
                found.push_back ({ bodyStart, tagPos, typeName, kindFromClassName (typeName) });
                prevEnd = tagPos + libTagLen + 20;
            }
        }
        else
        {
            // Synthetic / compact file without UTF-16 library tags: use ClassHits directly
            for (size_t i = 0; i < uniqueHits.size() && (int) found.size() < mostModules; ++i)
            {
                const size_t startPos = uniqueHits[i].pos;
                const size_t endPos = i + 1 < uniqueHits.size() ? uniqueHits[i + 1].pos : sliceEnd;
                found.push_back ({ startPos, endPos, uniqueHits[i].cls, kindFromClassName (uniqueHits[i].cls) });
            }
        }

        // Also detect PluginHost if present in uniqueHits but without a "Maschine 2.0 Library" tag
        for (auto& h : uniqueHits)
        {
            if (h.cls == "PluginHost")
            {
                const bool already = std::any_of (found.begin(), found.end(), [&] (const FoundMod& fm)
                {
                    return fm.kind == ModuleKind::pluginHost;
                });

                if (! already && (int) found.size() < mostModules)
                    found.push_back ({ h.pos, std::min (sliceEnd, h.pos + 64), "PluginHost", ModuleKind::pluginHost });
            }
        }

        // Parse each located module's zones and parameters
        for (auto& fm : found)
        {
            Module m;
            m.kind = fm.kind;
            m.typeName = fm.typeName;

            // Check Slot active flag immediately preceding bodyStart (if available)
            if (fm.bodyStart >= sliceStart + 12)
            {
                // Look back up to 24 bytes for the 4-byte active indicator (00 00 00 00 vs 01 01 01 01)
                for (size_t back = 4; back <= 24 && fm.bodyStart >= sliceStart + back; ++back)
                {
                    const size_t bp = fm.bodyStart - back;

                    if (d[bp] == 1 && d[bp + 1] == 1 && d[bp + 2] == 1 && d[bp + 3] == 1)
                    {
                        m.active = true;
                        break;
                    }
                }
            }

            if (m.kind == ModuleKind::sampler || m.kind == ModuleKind::audioModule)
            {
                size_t afterZones = fm.bodyStart;
                parseZones (d, fm.bodyStart, fm.metaPos, m.zones, afterZones);

                if (m.kind == ModuleKind::sampler && ! m.zones.empty() && afterZones < fm.metaPos)
                {
                    const auto eng = scanParams (d, afterZones, fm.metaPos, 64);

                    if (eng.size() > 24 && eng[24].isFloat)
                        m.samplerPitch = juce::jlimit (0.0f, 1.0f, eng[24].f);

                    if (eng.size() > 36)
                    {
                        if (! eng[34].isFloat && eng[34].i >= 0 && eng[34].i <= 3)
                            m.samplerFilterMode = eng[34].i;

                        if (eng[35].isFloat)
                            m.samplerCutoff = juce::jlimit (0.0f, 1.0f, eng[35].f);

                        if (eng[36].isFloat)
                            m.samplerResonance = juce::jlimit (0.0f, 1.0f, eng[36].f);
                    }
                }
            }
            else if (m.kind != ModuleKind::pluginHost && m.kind != ModuleKind::unknown)
            {
                const auto ps = scanParams (d, fm.bodyStart, fm.metaPos, 32);

                for (auto& rp : ps)
                {
                    if (rp.isFloat)
                        m.floatParams.push_back (rp.f);
                    else
                        m.intParams.push_back (rp.i);
                }
            }

            modules.push_back (std::move (m));
        }
    }

    /** Extracts Sound mixer Level (0..1, 0.75 = 0 dB) and Pan (-1..+1) from the Sound header
        preceding the Sound's name string. */
    void readSoundMixer (const uint8* d, size_t start, size_t namePos, float& level, float& pan) noexcept
    {
        level = 0.75f;
        pan = 0.0f;

        // Scan the Sound mixer block (~2650..3050 bytes before Sound name) for the Input Gain (0.75)
        // followed closely by Output Level and Pan FloatParameters.
        const size_t searchStart = namePos > 3050 ? std::max (start, namePos - 3050) : start;
        const size_t searchEnd = namePos > 2500 ? namePos - 2500 : namePos;

        if (searchStart >= searchEnd)
            return;

        std::vector<std::pair<size_t, float>> floats;

        for (size_t p = searchStart; p + 18 < searchEnd; ++p)
        {
            if (d[p] == 0 && d[p + 17] == 0)
            {
                const float f0 = readF32 (d, p + 1);
                const float f1 = readF32 (d, p + 5);

                if (juce::exactlyEqual (f0, f1) && std::abs (f0) <= 2.0f)
                {
                    // Verify preceded by at least 2 incrementing object_id bytes
                    if (p >= searchStart + 6)
                    {
                        floats.push_back ({ p, f0 });
                        p += 17;
                    }
                }
            }
        }

        // First float == 0.75 is Input Gain; ~50 bytes after it is Level; ~30 bytes after Level is Pan.
        for (size_t i = 0; i < floats.size(); ++i)
        {
            if (std::abs (floats[i].second - 0.75f) < 1.0e-4f)
            {
                for (size_t j = i + 1; j < floats.size(); ++j)
                {
                    const size_t dist = floats[j].first - floats[i].first;

                    if (dist >= 40 && dist <= 64 && floats[j].second >= 0.0f && floats[j].second <= 1.2f)
                        level = floats[j].second;
                    else if (dist >= 70 && dist <= 96 && std::abs (floats[j].second) <= 1.01f)
                        pan = floats[j].second;
                }

                break;
            }
        }
    }

    /** Infers DrumSynth sub-type from a pad's name when the slot only had a generic "Drumsynth" tag. */
    ModuleKind inferDrumSynthFromName (const juce::String& padName)
    {
        const auto lower = padName.toLowerCase();
        if (lower.contains ("kick") || lower.contains ("bd") || lower.contains ("bassdrum")) return ModuleKind::kick;
        if (lower.contains ("snare") || lower.contains ("sd") || lower.contains ("clap") || lower.contains ("rim")) return ModuleKind::snare;
        if (lower.contains ("hat") || lower.contains ("hh")) return ModuleKind::hihat;
        if (lower.contains ("tom")) return ModuleKind::tom;
        if (lower.contains ("cym") || lower.contains ("crash") || lower.contains ("ride")) return ModuleKind::cymbal;
        return ModuleKind::perc;
    }

    //==============================================================================
    // Writer helpers (for synthetic unit / round-trip / fuzz tests)

    void appendU16 (juce::MemoryBlock& b, uint16 v)
    {
        const uint8 bytes[2] { (uint8) (v & 0xff), (uint8) (v >> 8) };
        b.append (bytes, 2);
    }

    void appendU32 (juce::MemoryBlock& b, uint32 v)
    {
        const uint8 bytes[4] { (uint8) (v & 0xff), (uint8) ((v >> 8) & 0xff), (uint8) ((v >> 16) & 0xff), (uint8) (v >> 24) };
        b.append (bytes, 4);
    }

    void appendU64 (juce::MemoryBlock& b, uint64 v)
    {
        appendU32 (b, (uint32) (v & 0xffffffffu));
        appendU32 (b, (uint32) (v >> 32));
    }

    void appendF32 (juce::MemoryBlock& b, float v)
    {
        uint32 bits = 0;
        std::memcpy (&bits, &v, sizeof (bits));
        appendU32 (b, bits);
    }

    void appendPint (juce::MemoryBlock& b, int64_t v)
    {
        if (v == 0)
        {
            const uint8 z = 0;
            b.append (&z, 1);
            return;
        }

        const bool neg = v < 0;
        uint64 u = (uint64) (neg ? -v : v);
        uint8 bytes[8] {};
        int n = 0;

        while (u > 0)
        {
            bytes[n++] = (uint8) (u & 0xff);
            u >>= 8;
        }

        const int8_t sz = (int8_t) (neg ? -n : n);
        b.append (&sz, 1);
        b.append (bytes, (size_t) n);
    }

    void appendPString (juce::MemoryBlock& b, const juce::String& s)
    {
        const auto utf8 = s.toRawUTF8();
        const auto n = (int64_t) std::strlen (utf8);
        appendPint (b, n);

        if (n > 0)
            b.append (utf8, (size_t) n);
    }

    void appendOids (juce::MemoryBlock& b, int& oid, int count)
    {
        for (int i = 0; i < count; ++i)
            appendPint (b, oid++);
    }

    void appendFloatParam (juce::MemoryBlock& b, int& oid, float v, int oidCount = 4)
    {
        appendOids (b, oid, oidCount);
        const uint8 z = 0;
        b.append (&z, 1);
        appendF32 (b, v);
        appendF32 (b, v);
        appendF32 (b, 0.0f);
        appendF32 (b, 0.0f);
        b.append (&z, 1);
    }

    void appendIntParam (juce::MemoryBlock& b, int& oid, int v, int oidCount = 4)
    {
        appendOids (b, oid, oidCount);
        const uint8 z = 0;
        b.append (&z, 1);
        appendPint (b, v);
        appendPint (b, v);
        appendPint (b, 0);
        appendPint (b, 0);
        b.append (&z, 1);
    }

    juce::MemoryBlock makeFrame (const char* domain, uint32 id, const juce::MemoryBlock& inner, const juce::MemoryBlock& payload)
    {
        juce::MemoryBlock out;
        const uint64 total = (uint64) frameHeaderBytes + inner.getSize() + payload.getSize();
        appendU64 (out, total);
        out.append (domain, 4);
        appendU32 (out, id);
        appendU32 (out, 1);

        if (inner.getSize() > 0)
            out.append (inner.getData(), inner.getSize());

        if (payload.getSize() > 0)
            out.append (payload.getData(), payload.getSize());

        return out;
    }

    juce::MemoryBlock makeEndFrame()
    {
        juce::MemoryBlock out;
        appendU64 (out, 24);
        out.append (dataDomain, 4);
        appendU32 (out, endFrame);
        appendU32 (out, 1);
        appendU32 (out, 1);
        return out;
    }

    juce::MemoryBlock makeItem (const juce::MemoryBlock& frame, const std::vector<juce::MemoryBlock>& children = {})
    {
        juce::MemoryBlock tail;
        appendU32 (tail, 1);
        appendU32 (tail, (uint32) children.size());

        for (auto& c : children)
        {
            appendU32 (tail, 0);
            tail.append ("DSIN", 4);
            appendU32 (tail, 0);
            tail.append (c.getData(), c.getSize());
        }

        juce::MemoryBlock out;
        const uint64 total = (uint64) itemHeaderBytes + frame.getSize() + tail.getSize();
        appendU64 (out, total);
        appendU32 (out, 1);
        out.append (itemTag, 4);
        const uint8 zeros[24] {};
        out.append (zeros, 24);
        out.append (frame.getData(), frame.getSize());
        out.append (tail.getData(), tail.getSize());
        return out;
    }

    juce::String classNameForKind (ModuleKind k, const juce::String& fallback)
    {
        switch (k)
        {
            case ModuleKind::sampler:           return "Sampler";
            case ModuleKind::audioModule:       return "AudioModule";
            case ModuleKind::kick:              return "Kick";
            case ModuleKind::snare:             return "Snare";
            case ModuleKind::hihat:             return "Hihat";
            case ModuleKind::tom:               return "Tom";
            case ModuleKind::perc:              return "Perc";
            case ModuleKind::cymbal:            return "Cymbal";
            case ModuleKind::bassSynth:         return "BassSynth";
            case ModuleKind::eq:                return "EQ";
            case ModuleKind::filter:            return "Filter";
            case ModuleKind::saturator:         return "Saturator";
            case ModuleKind::distortion:        return "MullhollandDrive";
            case ModuleKind::compressor:        return "Compressor";
            case ModuleKind::maximizer:         return "Maximizer";
            case ModuleKind::limiter:           return "Limiter";
            case ModuleKind::transientDesigner: return "TransientDesigner";
            case ModuleKind::lofi:              return "LoFi";
            case ModuleKind::beatDelay:         return "BeatDelay";
            case ModuleKind::reflex:            return "Reflex";
            case ModuleKind::grainDelay:        return "GrainDelay";
            case ModuleKind::metaverb:          return "Metaverb";
            case ModuleKind::reverb:            return "Reverb";
            case ModuleKind::plateReverb:       return "PlateReverb";
            case ModuleKind::iceverb:           return "Iceverb";
            case ModuleKind::cabinet:           return "CabinetEmulation";
            case ModuleKind::gate:              return "Gate";
            case ModuleKind::chorus:            return "Chorus";
            case ModuleKind::flanger:           return "Flanger";
            case ModuleKind::phaser:            return "Phaser";
            case ModuleKind::freqShifter:       return "FreqShifter";
            case ModuleKind::fm:                return "FM";
            case ModuleKind::resochord:         return "Resochord";
            case ModuleKind::grainStretch:      return "GrainStretch";
            case ModuleKind::pluginHost:        return "PluginHost";
            case ModuleKind::unknown:           break;
        }

        return fallback.isNotEmpty() ? fallback : "Unknown";
    }
}

//==============================================================================
juce::String kindName (ModuleKind k)
{
    switch (k)
    {
        case ModuleKind::sampler:           return "Sampler";
        case ModuleKind::audioModule:       return "Audio";
        case ModuleKind::kick:              return "Kick Synth";
        case ModuleKind::snare:             return "Snare Synth";
        case ModuleKind::hihat:             return "Hi-Hat Synth";
        case ModuleKind::tom:               return "Tom Synth";
        case ModuleKind::perc:              return "Perc Synth";
        case ModuleKind::cymbal:            return "Cymbal Synth";
        case ModuleKind::bassSynth:         return "Bass Synth";
        case ModuleKind::eq:                return "EQ";
        case ModuleKind::filter:            return "Filter";
        case ModuleKind::saturator:         return "Saturator";
        case ModuleKind::distortion:        return "Distortion";
        case ModuleKind::compressor:        return "Compressor";
        case ModuleKind::maximizer:         return "Maximizer";
        case ModuleKind::limiter:           return "Limiter";
        case ModuleKind::transientDesigner: return "Transient Master";
        case ModuleKind::lofi:              return "Lo-Fi";
        case ModuleKind::beatDelay:         return "Beat Delay";
        case ModuleKind::reflex:            return "Reflex";
        case ModuleKind::grainDelay:        return "Grain Delay";
        case ModuleKind::metaverb:          return "Metaverb";
        case ModuleKind::reverb:            return "Reverb";
        case ModuleKind::plateReverb:       return "Plate Reverb";
        case ModuleKind::iceverb:           return "Ice Reverb";
        case ModuleKind::cabinet:           return "Cabinet";
        case ModuleKind::gate:              return "Gate";
        case ModuleKind::chorus:            return "Chorus";
        case ModuleKind::flanger:           return "Flanger";
        case ModuleKind::phaser:            return "Phaser";
        case ModuleKind::freqShifter:       return "Freq Shifter";
        case ModuleKind::fm:                return "FM";
        case ModuleKind::resochord:         return "Resochord";
        case ModuleKind::grainStretch:      return "Grain Stretch";
        case ModuleKind::pluginHost:        return "Plug-in";
        case ModuleKind::unknown:           break;
    }

    return "Module";
}

bool isSafeRelativePath (const juce::String& relativePath)
{
    if (relativePath.isEmpty() || relativePath.length() > 512)
        return false;

    const auto norm = relativePath.replaceCharacter ('\\', '/');

    if (norm.startsWithChar ('/') || (norm.length() >= 2 && norm[1] == ':'))
        return false;

    const auto parts = juce::StringArray::fromTokens (norm, "/", "");

    if (parts.isEmpty())
        return false;

    for (auto& part : parts)
    {
        if (part.isEmpty() || part == "." || part == "..")
            return false;

        for (auto p = part.getCharPointer(); ! p.isEmpty(); ++p)
            if (*p < 0x20)
                return false;
    }

    return true;
}

juce::File resolve (const Zone& zone, const juce::File& kitFile, const juce::File& library)
{
    if (! isSafeRelativePath (zone.relativePath))
        return {};

    const auto norm = zone.relativePath.replaceCharacter ('\\', '/');
    const auto parts = juce::StringArray::fromTokens (norm, "/", "");

    auto walkFrom = [&] (const juce::File& root) -> juce::File
    {
        if (root == juce::File())
            return {};

        auto cur = root;

        for (auto& p : parts)
            cur = cur.getChildFile (p);

        return cur;
    };

    // 1. Walk up from the kit file's directory (standard NI layout: <Library>/Groups/Kits/Foo.mxgrp -> <Library>/Samples/...)
    auto dir = kitFile.isDirectory() ? kitFile : kitFile.getParentDirectory();

    for (int up = 0; up < 5 && dir != juce::File(); ++up)
    {
        const auto candidate = walkFrom (dir);

        if (candidate.existsAsFile())
            return candidate;

        const auto parent = dir.getParentDirectory();

        if (parent == dir)
            break;

        dir = parent;
    }

    // 2. Check the kit's stored basePath directly (if it happens to exist on this machine)
    if (zone.basePath.isNotEmpty())
    {
        const juce::File base (zone.basePath.replaceCharacter ('\\', '/'));

        if (base.isDirectory())
        {
            const auto candidate = walkFrom (base);

            if (candidate.existsAsFile())
                return candidate;
        }
    }

    // 3. Look under a user-supplied library folder (full relative path, then longest trailing suffix)
    if (library != juce::File() && library.isDirectory())
    {
        for (int skip = 0; skip < parts.size(); ++skip)
        {
            auto cur = library;

            for (int i = skip; i < parts.size(); ++i)
                cur = cur.getChildFile (parts[i]);

            if (cur.existsAsFile())
                return cur;
        }
    }

    // Return the expected path under the kit's library root so missing-sample prompts show a clear path
    auto expectedRoot = kitFile.isDirectory() ? kitFile : kitFile.getParentDirectory();

    for (int up = 0; up < 2; ++up)
        if (expectedRoot.getParentDirectory() != expectedRoot && ! expectedRoot.getChildFile ("Samples").isDirectory())
            expectedRoot = expectedRoot.getParentDirectory();

    return walkFrom (expectedRoot);
}

//==============================================================================
juce::String read (const void* data, size_t size, Kit& kit)
{
    kit = {};

    if (data == nullptr || size < (size_t) itemHeaderBytes)
        return notAMaschineFile;

    if (size > mostFileBytes)
        return "This Maschine file is larger than the " + juce::File::descriptionOfSizeInBytes ((juce::int64) mostFileBytes) + " limit.";

    In whole { static_cast<const uint8*> (data), size, 0, true };

    // Verify outer hsin container
    {
        In probe = whole;
        const auto declared = probe.u64();
        probe.skip (4);

        if (! probe.ok || declared != size || ! probe.tagged (itemTag))
            return notAMaschineFile;
    }

    // Distinguish .mxgrp ("2SAM PRG") vs .mxsnd ("2SAM DNS") from outer child header
    kit.isGroup = true;

    for (size_t p = 40; p + 8 < std::min<size_t> (size, 512); ++p)
    {
        const auto* b = static_cast<const uint8*> (data) + p;

        if (std::memcmp (b, "2SAM DNS", 8) == 0)
        {
            kit.isGroup = false;
            break;
        }

        if (std::memcmp (b, "2SAM PRG", 8) == 0)
        {
            kit.isGroup = true;
            break;
        }
    }

    // Optional metadata frame 108 for the kit's display name
    {
        In probe = whole, meta;

        if (findInItem (probe, dataDomain, metaFrame, meta, 0) && meta.size > 24)
        {
            // First UTF-16LE string after the 16-byte header inside frame 108
            for (size_t p = 8; p + 8 < std::min<size_t> (meta.size, 64); p += 4)
            {
                juce::String nm;

                if (readUtf16 (meta.data, meta.size, p, nm, 80) && nm.length() >= 2)
                {
                    kit.name = nm;
                    break;
                }
            }
        }
    }

    // Required payload frame 109 carrying the Boost portable binary archive
    In archive;

    if (! findInItem (whole, dataDomain, archiveFrame, archive, 0))
        return notAMaschineFile;

    // Locate "serialization::archive" signature inside frame 109
    const char sig[] = "serialization::archive";
    constexpr size_t sigLen = sizeof (sig) - 1;
    size_t archOff = (size_t) -1;

    for (size_t p = 2; p + sigLen < std::min<size_t> (archive.size, 128); ++p)
    {
        if (archive.data[p] == 's' && std::memcmp (archive.data + p, sig, sigLen) == 0)
        {
            archOff = p - 2;
            break;
        }
    }

    if (archOff == (size_t) -1)
        return notAMaschineFile;

    const uint8* d = archive.data + archOff;
    const size_t n = archive.size - archOff;

    // 1. Build class_id -> NI::MASCHINE::DATA::<ClassName> map
    std::map<int, juce::String> classMap;
    const char nsPrefix[] = "NI::MASCHINE::DATA::";
    constexpr size_t nsLen = sizeof (nsPrefix) - 1;

    for (size_t p = 24; p + nsLen + 2 < n; ++p)
    {
        if (d[p] == 'N' && d[p + 1] == 'I' && std::memcmp (d + p, nsPrefix, nsLen) == 0)
        {
            size_t q = p + nsLen;

            while (q < n && ((d[q] >= 'A' && d[q] <= 'Z') || (d[q] >= 'a' && d[q] <= 'z') || (d[q] >= '0' && d[q] <= '9') || d[q] == '_'))
                ++q;

            const size_t fullLen = q - p;
            const juce::String shortName ((const char*) (d + p + nsLen), q - (p + nsLen));

            if (p >= 4 && d[p - 2] == 1 && (size_t) d[p - 1] == fullLen)
            {
                if (d[p - 4] == 1)
                    classMap[(int) d[p - 3]] = shortName;
                else if (p >= 5 && d[p - 5] == 2)
                    classMap[(int) (d[p - 4] | (d[p - 3] << 8))] = shortName;
            }

            p = q;
        }
    }

    // 2. Locate every Sound header (and in a .mxgrp, the Group entry right after Pad 16):
    //    Maschine 2.6+: 0x00 0x01 <len> <name> [oid0..oid3] 0x00 <17 bytes FloatParam> [oid4 == oid0 + 4]
    //    Maschine 2.0-2.4:   0x01 <len> <name> [oid0..oid2] <17 zero bytes> [oid3 == oid0 + 3][oid4 == oid0 + 4]
    struct Entry
    {
        size_t namePos = 0;
        size_t afterName = 0;
        juce::String name;
    };

    std::vector<Entry> entries;

    for (size_t p = 24; p + 32 < n && entries.size() < (size_t) mostPads + 8; ++p)
    {
        if (d[p] == 1 && d[p + 1] >= 1 && d[p + 1] <= 64)
        {
            const size_t slen = d[p + 1];

            if (p + 2 + slen + 28 < n && isPrintableString (d + p + 2, slen))
            {
                size_t cur = p + 2 + slen;
                int64_t v0 = 0, v1 = 0, v2 = 0;

                if (readPint (d, n, cur, v0) && v0 > 10
                    && readPint (d, n, cur, v1) && v1 == v0 + 1
                    && readPint (d, n, cur, v2) && v2 == v0 + 2)
                {
                    // Variant A (Maschine 2.6+): 4 sequential IDs + 0x00 + 17 bytes + v0+4
                    size_t curA = cur;
                    int64_t v3 = 0, v4 = 0;
                    if (d[p - 1] == 0
                        && readPint (d, n, curA, v3) && v3 == v0 + 3
                        && curA + 18 < n && d[curA] == 0)
                    {
                        size_t checkNext = curA + 1 + 17;
                        if (readPint (d, n, checkNext, v4) && v4 == v0 + 4)
                        {
                            entries.push_back ({ p + 2, curA, juce::String::fromUTF8 ((const char*) (d + p + 2), (int) slen) });
                            p = checkNext;
                            continue;
                        }
                    }

                    // Variant B (Maschine 2.0-2.4): 3 sequential IDs + 17-byte FloatParam + v0+3 + v0+4
                    if (cur + 17 < n)
                    {
                        size_t checkNext = cur + 17;
                        int64_t n3 = 0, n4 = 0;
                        if (readPint (d, n, checkNext, n3) && n3 == v0 + 3
                            && readPint (d, n, checkNext, n4) && n4 == v0 + 4)
                        {
                            entries.push_back ({ p + 2, cur, juce::String::fromUTF8 ((const char*) (d + p + 2), (int) slen) });
                            p = checkNext;
                            continue;
                        }
                    }
                }
            }
        }
    }

    if (entries.empty())
        return damagedFile;

    // 3. Parse Sounds (16 for .mxgrp, 1 for .mxsnd) and optional Group FX entry
    const size_t maxSounds = kit.isGroup ? std::min<size_t> (16, entries.size()) : 1;

    for (size_t i = 0; i < maxSounds; ++i)
    {
        Sound s;
        s.index = (int) i;
        s.name = entries[i].name;

        const size_t prevBoundary = i > 0 ? entries[i - 1].afterName : 24;
        readSoundMixer (d, prevBoundary, entries[i].namePos, s.level, s.pan);

        const size_t sliceEnd = (i + 1 < entries.size()) ? entries[i + 1].namePos : n;
        parseModulesInSlice (d, entries[i].afterName, sliceEnd, classMap, s.modules);

        // Resolve any generic "Drumsynth" label using pad name
        for (auto& m : s.modules)
            if (m.kind == ModuleKind::unknown && m.typeName.equalsIgnoreCase ("Drumsynth"))
                m.kind = inferDrumSynthFromName (s.name);

        kit.sounds.push_back (std::move (s));
    }

    // 3b. Extract embedded MIDI NoteEvents (960 PPQN) inside each Sound's slice!
    int noteEventClassId = -1;

    for (auto& [cid, cname] : classMap)
        if (cname == "NoteEvent")
            noteEventClassId = cid;

    if (kit.isGroup && noteEventClassId >= 0)
    {
        // Pattern header hint (name, length, BPM) from entries after Group entry (#16)
        juce::String patName = kit.name.isNotEmpty() ? (kit.name + " Pattern") : "Pattern 1";
        double patBpm = 0.0;
        double patLenBeats = 16.0;

        if (entries.size() > 17)
        {
            patName = entries[17].name;
            const auto lower = patName.toLowerCase();
            const int bpmIdx = lower.indexOf ("bpm");

            if (bpmIdx > 0)
            {
                // Extract digits immediately preceding "bpm"
                int j = bpmIdx - 1;
                while (j >= 0 && patName[j] == ' ') --j;
                int endDig = j + 1;
                while (j >= 0 && patName[j] >= '0' && patName[j] <= '9') --j;
                const auto num = patName.substring (j + 1, endDig).getDoubleValue();
                if (num >= 40.0 && num <= 240.0)
                    patBpm = num;
            }
        }

        Pattern pat;
        pat.name = patName;
        pat.bpm = patBpm;
        double maxEndBeat = 0.0;

        // Parse NoteEvent streams inside each Sound's slice [entries[i].afterName, nextEntry)
        for (size_t i = 0; i < maxSounds; ++i)
        {
            const size_t sStart = entries[i].afterName;
            const size_t sEnd = (i + 1 < entries.size()) ? entries[i + 1].namePos : n;

            // Scan for every NoteEvent instance:
            //   First occurrence: "NI::MASCHINE::DATA::NoteEvent" 01 01 00 [oid] 01 01 00 [oid] 01 01 00 [oid] 00 <dur> <pitch> <vel>
            //   Subsequent: <tick_pint> 01 <noteEventClassId> [oid][oid][oid] 00 <dur> <pitch> <vel>
            size_t p = sStart;
            int64_t curTick = 0;
            int notesForPad = 0;

            while (p + 12 < sEnd && notesForPad < 512)
            {
                bool matched = false;
                size_t q = p;

                if (d[p] == 'N' && p + 32 < sEnd && std::memcmp (d + p, "NI::MASCHINE::DATA::NoteEvent", 29) == 0)
                {
                    q = p + 29;
                    // Skip class tracking/version (01 01 00) and 3 tracked fields until 0x00 before dur/pitch/vel
                    // Pattern verified: 01 01 00 [oid] 01 01 00 [oid] 01 01 00 [oid] 00 <dur> <pitch> <vel>
                    for (int step = 0; step < 3 && q + 4 < sEnd; ++step)
                    {
                        q += 3;
                        int64_t dummy = 0;
                        if (! readPint (d, sEnd, q, dummy)) break;
                    }
                    if (q < sEnd && d[q] == 0)
                    {
                        ++q;
                        curTick = 0;
                        matched = true;
                    }
                }
                else if (d[p] == 1 && (int) d[p + 1] == noteEventClassId)
                {
                    // Check preceding 1..3 bytes for tick pint and verify 3 sequential oids follow
                    size_t afterCid = p + 2;
                    int64_t o0 = 0, o1 = 0, o2 = 0;

                    if (readPint (d, sEnd, afterCid, o0) && o0 > 100
                        && readPint (d, sEnd, afterCid, o1) && o1 == o0 + 1
                        && readPint (d, sEnd, afterCid, o2) && o2 == o0 + 2
                        && afterCid < sEnd && d[afterCid] == 0)
                    {
                        // Read tick pint immediately preceding p (1-byte 0x00, or 2-byte 0x01 xx, or 3-byte 0x02 xx yy)
                        int64_t tick = -1;
                        if (p >= sStart + 3 && d[p - 3] == 2)
                            tick = (int64_t) (d[p - 2] | (d[p - 1] << 8));
                        else if (p >= sStart + 2 && d[p - 2] == 1)
                            tick = (int64_t) d[p - 1];
                        else if (p >= sStart + 1 && d[p - 1] == 0)
                            tick = 0;

                        if (tick >= 0 && tick <= 61440)
                        {
                            curTick = tick;
                            q = afterCid + 1;
                            matched = true;
                        }
                    }
                }

                if (matched)
                {
                    int64_t dur = 0, pitch = 0, vel = 0;

                    if (readPint (d, sEnd, q, dur) && readPint (d, sEnd, q, pitch) && readPint (d, sEnd, q, vel)
                        && dur >= 1 && dur <= 30720 && pitch >= 0 && pitch <= 127 && vel >= 1 && vel <= 127)
                    {
                        PatternNote pn;
                        pn.soundIndex = (int) i;
                        pn.startBeat = (double) curTick / 960.0;
                        pn.durationBeats = juce::jmax (0.0625, (double) dur / 960.0);
                        pn.pitch = (int) pitch;
                        pn.velocity = (int) vel;
                        maxEndBeat = std::max (maxEndBeat, pn.startBeat + pn.durationBeats);
                        pat.notes.push_back (pn);
                        ++notesForPad;
                        p = q;
                        continue;
                    }
                }

                ++p;
            }
        }

        if (! pat.notes.empty())
        {
            // Snap pattern length up to a clean multiple of 4 beats (bars)
            patLenBeats = std::max (4.0, std::ceil (maxEndBeat / 4.0) * 4.0);
            pat.lengthBeats = patLenBeats;
            std::sort (pat.notes.begin(), pat.notes.end(), [] (const PatternNote& a, const PatternNote& b)
            {
                return std::tie (a.startBeat, a.soundIndex) < std::tie (b.startBeat, b.soundIndex);
            });
            kit.patterns.push_back (std::move (pat));
        }
    }

    if (kit.isGroup && entries.size() > 16)
    {
        if (kit.name.isEmpty())
            kit.name = entries[16].name;

        const size_t grpEnd = (entries.size() > 17) ? entries[17].namePos : n;
        parseModulesInSlice (d, entries[16].afterName, grpEnd, classMap, kit.groupFx);
    }

    return {};
}

juce::String read (const juce::File& file, Kit& kit)
{
    kit = {};

    if (! file.existsAsFile())
        return "Could not open " + file.getFileName() + ".";

    const auto length = file.getSize();

    if (length <= 0)
        return notAMaschineFile;

    if ((size_t) length > mostFileBytes)
        return "This Maschine file is larger than the " + juce::File::descriptionOfSizeInBytes ((juce::int64) mostFileBytes) + " limit.";

    juce::MemoryBlock bytes;

    if (! file.loadFileAsData (bytes))
        return "Could not read " + file.getFileName() + ".";

    const auto err = read (bytes.getData(), bytes.getSize(), kit);

    if (err.isEmpty() && kit.name.isEmpty())
        kit.name = file.getFileNameWithoutExtension();

    return err;
}

//==============================================================================
juce::MemoryBlock write (const Kit& kit)
{
    juce::MemoryBlock arch;
    // Standard 24-byte prefix before portable_binary_iarchive signature
    appendU32 (arch, 1);
    appendU32 (arch, 1);
    appendU32 (arch, 1);
    appendU32 (arch, 0);
    appendU32 (arch, 0);
    appendU32 (arch, 0);
    appendPString (arch, "serialization::archive");
    appendPint (arch, 17);
    const uint8 zero = 0, one = 1;
    arch.append (&zero, 1);
    appendPint (arch, 1);

    int oid = 200;
    std::map<juce::String, int> registeredClassId;
    int nextClassId = 40;

    auto writeModule = [&] (const Module& m)
    {
        const auto cls = classNameForKind (m.kind, m.typeName);
        const auto full = "NI::MASCHINE::DATA::" + cls;

        // Active marker
        const uint8 activeBytes[4] { (uint8) (m.active ? 1 : 0), (uint8) (m.active ? 1 : 0),
                                     (uint8) (m.active ? 1 : 0), (uint8) (m.active ? 1 : 0) };
        arch.append (activeBytes, 4);
        const uint8 padZ[4] {};
        arch.append (padZ, 4);

        auto it = registeredClassId.find (cls);

        if (it == registeredClassId.end())
        {
            const int cid = nextClassId++;
            registeredClassId[cls] = cid;
            arch.append (&one, 1);
            arch.append (&one, 1);
            appendPint (arch, cid);
            appendPString (arch, full);
            arch.append (&one, 1);
            arch.append (&one, 1);
            appendPint (arch, 2);
            appendPint (arch, oid++);
        }
        else
        {
            appendPint (arch, it->second);
            appendPint (arch, oid++);
        }

        if (m.kind == ModuleKind::sampler || m.kind == ModuleKind::audioModule)
        {
            for (auto& z : m.zones)
            {
                appendPString (arch, z.relativePath);
                arch.append (&zero, 1);
                arch.append (&zero, 1);
                appendPString (arch, z.basePath.isNotEmpty() ? z.basePath : "/Library/");
                appendPString (arch, z.bankName.isNotEmpty() ? z.bankName : "Test Bank");

                // 21 zone params (indices 0..20)
                appendIntParam (arch, oid, z.startFrame);     // 0
                appendIntParam (arch, oid, z.endFrame);       // 1
                appendIntParam (arch, oid, 0);                // 2
                appendIntParam (arch, oid, z.endFrame);       // 3
                appendIntParam (arch, oid, 0);                // 4
                appendIntParam (arch, oid, 0);                // 5
                appendIntParam (arch, oid, 0);                // 6
                appendIntParam (arch, oid, 0);                // 7
                appendIntParam (arch, oid, z.rootKey);        // 8
                appendIntParam (arch, oid, z.lowKey);         // 9
                appendIntParam (arch, oid, z.highKey);        // 10
                appendIntParam (arch, oid, z.lowVelocity);    // 11
                appendIntParam (arch, oid, z.highVelocity);   // 12
                appendFloatParam (arch, oid, z.gain);         // 13
                appendFloatParam (arch, oid, z.pan);          // 14
                appendFloatParam (arch, oid, z.tune);         // 15
                appendIntParam (arch, oid, 0);                // 16
                appendIntParam (arch, oid, 0);                // 17
                appendIntParam (arch, oid, 0);                // 18
                appendIntParam (arch, oid, z.totalFrames);    // 19
                appendIntParam (arch, oid, 0);                // 20
            }

            if (m.kind == ModuleKind::sampler)
            {
                // Pad indices 21..36 for Sampler engine pitch & filter
                appendIntParam (arch, oid, 0);                        // 21
                appendIntParam (arch, oid, 4);                        // 22
                appendFloatParam (arch, oid, 0.0f);                   // 23
                appendFloatParam (arch, oid, m.samplerPitch);         // 24
                for (int k = 25; k <= 33; ++k)
                    appendFloatParam (arch, oid, 0.0f);
                appendIntParam (arch, oid, m.samplerFilterMode);      // 34
                appendFloatParam (arch, oid, m.samplerCutoff);        // 35
                appendFloatParam (arch, oid, m.samplerResonance);     // 36
            }
        }
        else
        {
            for (auto iv : m.intParams)
                appendIntParam (arch, oid, iv);

            for (auto fv : m.floatParams)
                appendFloatParam (arch, oid, fv);
        }

        // Trailing UTF-16 "Maschine 2.0 Library" + module tag
        const juce::String libStr ("Maschine 2.0 Library");

        for (int i = 0; i < libStr.length(); ++i)
            appendU16 (arch, (uint16) libStr[i]);

        appendU32 (arch, (uint32) cls.length());

        for (int i = 0; i < cls.length(); ++i)
            appendU16 (arch, (uint16) cls[i]);
    };

    auto writeSound = [&] (const Sound& s)
    {
        // Mixer floats ~2850 bytes before Sound name
        const size_t mixerBase = arch.getSize();
        appendFloatParam (arch, oid, 0.75f);
        // ~50 bytes gap
        while (arch.getSize() < mixerBase + 52)
            arch.append (&zero, 1);
        appendFloatParam (arch, oid, s.level);
        while (arch.getSize() < mixerBase + 82)
            arch.append (&zero, 1);
        appendFloatParam (arch, oid, s.pan);

        // Pad to ~2900 bytes before Sound name
        const size_t targetNameOff = mixerBase + 2900;

        while (arch.getSize() < targetNameOff)
            arch.append (&zero, 1);

        // Sound name header: 0x00 + PString + 4 sequential oids + 0x00 + 17 bytes + oid+4
        arch.append (&zero, 1);
        appendPString (arch, s.name.isNotEmpty() ? s.name : ("Pad " + juce::String (s.index + 1)));
        appendFloatParam (arch, oid, 0.0f, 4);
        appendIntParam (arch, oid, 0, 5);
        appendIntParam (arch, oid, 0, 4);
        appendIntParam (arch, oid, 1, 4);

        for (auto& m : s.modules)
            writeModule (m);
    };

    for (auto& s : kit.sounds)
        writeSound (s);

    // If it's a group and has group FX, pad to 16 sounds then write group entry
    if (kit.isGroup && ! kit.groupFx.empty())
    {
        for (int i = (int) kit.sounds.size(); i < 16; ++i)
            writeSound ({ i, "Empty " + juce::String (i + 1), 0.75f, 0.0f, {} });

        Sound grp;
        grp.index = 16;
        grp.name = kit.name.isNotEmpty() ? kit.name : "Group";
        grp.modules = kit.groupFx;
        writeSound (grp);
    }

    // Wrap inside hsin / DSIN container with domain tag ("2SAM PRG" or "2SAM DNS") + frame 109
    const auto payloadItem = makeItem (makeFrame (dataDomain, archiveFrame, makeEndFrame(), arch));
    const auto innerItem = makeItem (makeFrame (dataDomain, 101, makeEndFrame(), {}), { payloadItem });

    juce::MemoryBlock rootTail;
    appendU32 (rootTail, 1);
    appendU32 (rootTail, 1);
    appendU32 (rootTail, 0);
    rootTail.append (kit.isGroup ? "2SAM PRG" : "2SAM DNS", 8);
    rootTail.append (innerItem.getData(), innerItem.getSize());

    const auto rootFrame = makeFrame (dataDomain, 118, makeEndFrame(), {});
    juce::MemoryBlock out;
    const uint64 total = (uint64) itemHeaderBytes + rootFrame.getSize() + rootTail.getSize();
    appendU64 (out, total);
    appendU32 (out, 1);
    out.append (itemTag, 4);
    const uint8 zeros[24] {};
    out.append (zeros, 24);
    out.append (rootFrame.getData(), rootFrame.getSize());
    out.append (rootTail.getData(), rootTail.getSize());
    return out;
}

juce::MidiFile buildMidiFile (const Pattern& pat, const std::map<int, int>& soundToNote, double fallbackBpm)
{
    constexpr int tpq = 960;
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (tpq);

    juce::MidiMessageSequence seq;
    const double bpm = pat.bpm > 0.0 ? pat.bpm : juce::jlimit (40.0, 240.0, fallbackBpm);
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::llround (60000000.0 / bpm)), 0.0);

    if (pat.name.isNotEmpty())
        seq.addEvent (juce::MidiMessage::textMetaEvent (3, pat.name), 0.0);

    for (auto& n : pat.notes)
    {
        const auto it = soundToNote.find (n.soundIndex);
        const int midiNote = juce::jlimit (0, 127, it != soundToNote.end() ? it->second : (36 + n.soundIndex));
        const double onTick = juce::jmax (0.0, n.startBeat * (double) tpq);
        const double offTick = onTick + juce::jmax (30.0, n.durationBeats * (double) tpq);
        const auto vel = (juce::uint8) juce::jlimit (1, 127, n.velocity);

        seq.addEvent (juce::MidiMessage::noteOn (10, midiNote, vel), onTick);
        seq.addEvent (juce::MidiMessage::noteOff (10, midiNote), offTick);
    }

    seq.sort();
    seq.updateMatchedPairs();
    mf.addTrack (seq);
    return mf;
}

juce::String exportPatternToMidi (const Pattern& pat, const std::map<int, int>& soundToNote, const juce::File& outFile, double fallbackBpm)
{
    if (pat.notes.empty())
        return "This Maschine pattern has no notes to export.";

    const auto mf = buildMidiFile (pat, soundToNote, fallbackBpm);
    juce::FileOutputStream out (outFile);

    if (! out.openedOk())
        return "Could not create " + outFile.getFileName() + ".";

    out.setPosition (0);
    out.truncate();

    if (! mf.writeTo (out))
        return "Could not write MIDI data to " + outFile.getFileName() + ".";

    return {};
}
}
