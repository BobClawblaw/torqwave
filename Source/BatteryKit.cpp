#include "BatteryKit.h"

namespace BatteryKit
{
namespace
{
    using juce::uint8;
    using juce::uint16;
    using juce::uint32;
    using juce::uint64;

    constexpr char itemTag[] = "hsin", dataDomain[] = "DSIN", kontaktDomain[] = "4KIN";

    // The frames that matter: the block (packed or not), and inside it the program's records.
    constexpr uint32 endFrame = 1, blockFrame = 115, recordsFrame = 109;

    // The records.
    constexpr int programRecord = 0x28, voiceGroupsRecord = 0x32, cellsRecord = 0x33, zonesRecord = 0x34, pathsRecord = 0x4b;

    // Inside a cell: its modulators, each a record in a list; an envelope's holder and its values.
    constexpr int modulatorsRecord = 0x3b, modulatorRecord = 0x0d, envelopeHolderRecord = 0x07, envelopeRecord = 0x3f;
    constexpr char volumeTarget[] = "volume", volumeEnvelopeName[] = "ENV_AHDSR_VOLUME";

    // The versions of each this has been checked against. A program's own layout is not read, only
    // its children, each of which is checked for itself, so any version in the range seen is taken.
    constexpr int oldestProgram = 0xa3, newestProgram = 0xb2, cellVersion = 0x95, zoneVersion = 0x98, zoneVersionWithMore = 0x9a,
                  voiceGroupVersion = 0x60, pathsVersion = 2, pathsVersionWithKit = 3;

    // In the newer path table every path is followed by this many bytes about its file.
    constexpr size_t pathFileBytes = 28;

    // A cell's private part starts with a table of this many bytes; its flags, colour and keys follow.
    constexpr size_t cellTableBytes = 1632, cellColourAt = cellTableBytes + 3, cellLowKeyAt = cellColourAt + 4, cellHighKeyAt = cellLowKeyAt + 4,
                     cellPrivateBytes = cellHighKeyAt + 4;

    constexpr int itemHeaderBytes = 40, frameHeaderBytes = 20, mostDepth = 16;

    // The unpacker's far back-references start past this distance.
    constexpr size_t nearDistance = 8191;

    const juce::String notAKit = "This is not a Battery 4 kit or cell file.";
    const juce::String damaged = "The kit file is damaged or cut short.";

    juce::String unknownVersion (const juce::String& what, int version)
    {
        return "This kit was saved by a version of Battery this cannot read yet (its " + what + " record is version " + juce::String (version) + ").";
    }

    //==============================================================================
    /** A range of the file's bytes, read in order. Reading past its end reads zeros and leaves it
        not ok, so a caller can read a whole record and ask once. */
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

        uint8 u8() noexcept
        {
            return need (1) ? data[at++] : (uint8) 0;
        }

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

            const auto v = (uint32) data[at] | ((uint32) data[at + 1] << 8) | ((uint32) data[at + 2] << 16) | ((uint32) data[at + 3] << 24);
            at += 4;
            return v;
        }

        uint64 u64() noexcept
        {
            const auto low = (uint64) u32();
            return low | ((uint64) u32() << 32);
        }

        int i16() noexcept                          { return (int) (juce::int16) u16(); }
        int i32() noexcept                          { return (int) (juce::int32) u32(); }

        /** A number that is not one (an infinity, a NaN) reads as `otherwise`. */
        float f32 (float otherwise) noexcept
        {
            const auto bits = u32();
            float v;
            std::memcpy (&v, &bits, sizeof (v));
            return std::isfinite (v) ? v : otherwise;
        }

        /** The next n bytes as a range of their own. */
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

        /** A short name in ASCII: its length, then its bytes. */
        juce::String ascii()
        {
            const auto length = u32();

            if (length > 64 || ! need (length))
            {
                ok = false;
                return {};
            }

            juce::String s;

            for (uint32 i = 0; i < length; ++i)
            {
                const auto c = (char) data[at++];
                s << (c >= 32 && c < 127 ? c : '?');
            }

            return s;
        }

        /** Text: its length in characters, then UTF-16. */
        juce::String text()
        {
            const auto length = u32();

            if (length > (uint32) mostNameLength || ! need ((size_t) length * 2))
            {
                ok = false;
                return {};
            }

            std::vector<juce::juce_wchar> chars;
            chars.reserve ((size_t) length + 1);

            for (uint32 i = 0; i < length; ++i)
            {
                juce::juce_wchar c = u16();

                if (c >= 0xd800 && c <= 0xdbff && i + 1 < length)
                {
                    const auto next = (juce::juce_wchar) (data[at] | (data[at + 1] << 8));

                    if (next >= 0xdc00 && next <= 0xdfff)
                    {
                        c = 0x10000 + ((c - 0xd800) << 10) + (next - 0xdc00);
                        at += 2;
                        ++i;
                    }
                }

                // Half of a pair on its own, or a character that ends a text, is not kept.
                if (c == 0 || (c >= 0xd800 && c <= 0xdfff))
                    c = 0xfffd;

                chars.push_back (c);
            }

            chars.push_back (0);
            return juce::String (juce::CharPointer_UTF32 (chars.data()));
        }
    };

    /** A record's body when it is in three parts: private, public, and its children. */
    struct Parts
    {
        int version = 0;
        In priv, pub, children;
    };

    bool readParts (In& in, Parts& p)
    {
        if (in.u8() != 1)
            in.ok = false;

        p.version = in.u16();
        p.priv = in.take (in.u32());
        p.pub = in.take (in.u32());
        p.children = in.take (in.u32());
        return in.ok;
    }

    //==============================================================================
    // The container. An item: its size, a tag, an id, then its frames (each wrapping the next, the
    // innermost an end frame), then its children, each an item.

    /** Looks through a frame and the ones inside it for the one wanted; `frame` is the whole of it. */
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

        // The frame inside comes first; what is left after it is this one's own.
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

    /** Looks through an item and its children for a frame; `item` starts at the item and may run past it. */
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
    /** A cell's volume envelope, from its modulators. Left unread (and the cell says so) where
        they are not laid out as this knows: an envelope is not worth refusing a kit for. */
    void readEnvelope (In list, Cell& c)
    {
        list.skip (3);

        while (list.left() > 0 && list.u8() == 1)
        {
            const auto id = (int) list.u16();
            auto body = list.take (list.u32());
            Parts modulator;

            if (! list.ok)
                return;

            if (id != modulatorRecord || ! readParts (body, modulator))
                continue;

            auto& priv = modulator.priv;
            priv.skip (4);
            const auto target = priv.ascii();
            priv.skip (9);
            const auto name = priv.ascii();
            priv.skip (3);
            const auto bypassed = priv.u8() != 0;

            if (! priv.ok || target != volumeTarget || name != volumeEnvelopeName)
                continue;

            for (auto& children = modulator.children; children.left() > 0;)
            {
                const auto child = (int) children.u16();
                auto holder = children.take (children.u32());
                Parts held;

                if (! children.ok)
                    return;

                if (child != envelopeHolderRecord || ! readParts (holder, held))
                    continue;

                while (held.children.left() > 0)
                {
                    const auto inner = (int) held.children.u16();
                    auto values = held.children.take (held.children.u32());

                    if (! held.children.ok)
                        return;

                    if (inner != envelopeRecord)
                        continue;

                    // A curve, then attack, decay and hold, in ms.
                    values.skip (7);
                    const auto attack = values.f32 (0.0f), decay = values.f32 (0.0f), hold = values.f32 (0.0f);

                    if (! values.ok)
                        return;

                    c.envelopeRead = true;
                    c.envelopeOn = ! bypassed;
                    c.attack = juce::jlimit (0.0f, 60000.0f, attack);
                    c.hold = juce::jlimit (0.0f, 60000.0f, hold);
                    c.decay = juce::jlimit (0.0f, 60000.0f, decay);
                    return;
                }
            }

            return;
        }
    }

    juce::String readCells (In list, std::vector<Cell>& cells)
    {
        const auto count = list.u32();

        if (! list.ok || count > (uint32) mostCells)
            return damaged;

        cells.reserve (count);

        for (uint32 i = 0; i < count; ++i)
        {
            Parts parts;

            if (! readParts (list, parts))
                return damaged;

            if (parts.version != cellVersion)
                return unknownVersion ("cell", parts.version);

            Cell c;
            c.index = (int) i;
            auto& pub = parts.pub;
            c.name = pub.text().trim();
            c.gain = juce::jlimit (0.0f, 16.0f, pub.f32 (1.0f));
            c.pan = juce::jlimit (-1.0f, 1.0f, pub.f32 (0.0f));
            c.tune = juce::jlimit (1.0f / 64.0f, 64.0f, pub.f32 (1.0f));
            c.keyTracking = pub.u8() != 0;
            c.reverse = pub.u8() != 0;
            pub.skip (8);
            const auto group = pub.i32();

            if (! pub.ok)
                return damaged;

            // The first voice group is the kit's own; a cell names the ones after it from 1.
            c.voiceGroup = group >= 1 ? group - 1 : -1;

            if (parts.priv.size >= cellPrivateBytes)
            {
                auto priv = parts.priv;
                priv.skip (cellColourAt);
                const auto colour = priv.i32(), low = priv.i32(), high = priv.i32();
                c.colour = juce::isPositiveAndBelow (colour, 256) ? colour : -1;

                if (juce::isPositiveAndBelow (low, 128) && juce::isPositiveAndBelow (high, 128) && low <= high)
                {
                    c.lowKey = low;
                    c.highKey = high;
                }
            }

            for (auto& children = parts.children; children.left() > 0;)
            {
                const auto child = (int) children.u16();
                const auto body = children.take (children.u32());

                if (! children.ok)
                    return damaged;

                if (child == modulatorsRecord && ! c.envelopeRead)
                    readEnvelope (body, c);
            }

            cells.push_back (std::move (c));
        }

        return {};
    }

    juce::String readZones (In list, std::vector<Cell>& cells)
    {
        const auto count = list.u32();

        if (! list.ok || count > (uint32) mostZones)
            return damaged;

        for (uint32 i = 0; i < count; ++i)
        {
            const auto cell = list.u32();
            Parts parts;

            if (! readParts (list, parts))
                return damaged;

            if (parts.version != zoneVersion && parts.version != zoneVersionWithMore)
                return unknownVersion ("zone", parts.version);

            if (cell >= (uint32) cells.size())
                return damaged;

            Layer l;
            auto& pub = parts.pub;
            const auto start = pub.i32(), end = pub.i32();
            pub.skip (4);
            const auto lowVelocity = pub.i16(), highVelocity = pub.i16(), lowKey = pub.i16(), highKey = pub.i16();
            const auto fadeLow = pub.i16(), fadeHigh = pub.i16();
            pub.skip (4);
            const auto root = pub.i16();
            l.gain = juce::jlimit (0.0f, 16.0f, pub.f32 (1.0f));
            l.pan = juce::jlimit (-1.0f, 1.0f, pub.f32 (0.0f));
            l.tune = juce::jlimit (1.0f / 64.0f, 64.0f, pub.f32 (1.0f));

            if (parts.version == zoneVersionWithMore)
                pub.skip (6);

            l.path = pub.i32();
            pub.skip (9);
            const auto frames = pub.i32();

            if (! pub.ok)
                return damaged;

            l.lowVelocity = juce::jlimit (1, 127, lowVelocity);
            l.highVelocity = juce::jlimit (l.lowVelocity, 127, highVelocity);
            l.lowKey = juce::jlimit (0, 127, lowKey);
            l.highKey = juce::jlimit (l.lowKey, 127, highKey);
            l.rootKey = juce::jlimit (0, 127, root);
            l.fadeLow = juce::jlimit (0, 127, fadeLow);
            l.fadeHigh = juce::jlimit (0, 127, fadeHigh);
            l.frames = juce::jmax (0, frames);
            l.start = juce::jlimit (0, l.frames, start);

            // An end is kept as how far back from the sample's end it is: nothing, or less than nothing.
            l.endBack = (int) juce::jlimit ((juce::int64) 0, (juce::int64) (l.frames - l.start), -(juce::int64) juce::jmin (0, end));
            cells[cell].layers.push_back (l);
        }

        return {};
    }

    /** False when the record is not laid out as this knows: then the kit has no voice groups.
        The kit's own group comes first, then sixteen bytes with a bit for each of the 128 groups
        that is kept, then those groups in order. */
    bool readVoiceGroups (In list, Kit& kit)
    {
        const auto readOne = [&list] (VoiceGroup& g)
        {
            if (list.u8() != 0 || list.u16() != voiceGroupVersion)
                return false;

            g.name = list.text();
            list.skip (3);
            g.voices = juce::jlimit (0, 4096, list.i32());
            list.skip (4);
            g.exclusiveGroup = juce::jmax (-1, list.i32());
            return list.ok;
        };

        VoiceGroup own;

        if (! readOne (own))
            return false;

        std::array<uint8, numVoiceGroups / 8> kept {};

        for (auto& byte : kept)
            byte = list.u8();

        if (! list.ok)
            return false;

        std::vector<VoiceGroup> groups;

        for (int i = 0; i < numVoiceGroups; ++i)
        {
            if ((kept[(size_t) (i / 8)] & (1 << (i % 8))) == 0)
                continue;

            groups.resize ((size_t) i + 1);

            if (! readOne (groups[(size_t) i]))
                return false;
        }

        if (list.left() > 0)
            return false;

        kit.kitVoices = own.voices;
        kit.voiceGroups = std::move (groups);
        return true;
    }

    /** The table of paths. The older one is a count and the paths, and the zones name them from 0.
        The newer one (what a current Battery 4 saves) starts with the kit file itself, so the zones name
        the samples from 1, says one more than how many paths it holds, and follows each path with
        some bytes about its file. kitFirst says which it was. */
    juce::String readPaths (In table, std::vector<Path>& paths, bool& kitFirst)
    {
        constexpr int upPart = 3, folderPart = 2, filePart = 4;
        const auto version = (int) table.u16();

        if (version != pathsVersion && version != pathsVersionWithKit)
            return table.ok ? unknownVersion ("sample path", version) : damaged;

        kitFirst = version == pathsVersionWithKit;
        uint32 count = 0;

        if (kitFirst)
        {
            const auto onePast = table.u32();
            table.skip (8);

            if (onePast == 0)
                return damaged;

            count = onePast - 1;
        }
        else
        {
            table.skip (8);
            count = table.u32();
        }

        if (! table.ok || count > (uint32) mostPaths)
            return damaged;

        paths.reserve (juce::jmin ((size_t) count, table.left() / 4));

        for (uint32 i = 0; i < count; ++i)
        {
            const auto parts = table.u32();

            if (! table.ok || parts > (uint32) mostPathParts)
                return damaged;

            Path p;

            for (uint32 j = 0; j < parts; ++j)
            {
                const auto kind = (int) table.u8();

                if (kind == upPart)
                {
                    // Up is only read at the front, as Battery writes it.
                    if (p.parts.isEmpty())
                        ++p.up;
                    else
                        return damaged;
                }
                else if (kind == folderPart || kind == filePart)
                {
                    p.parts.add (table.text());
                }
                else
                {
                    // A kind of part this does not know has no length to step over: the paths
                    // from here on are left empty, and their layers come over without a sample.
                    paths.resize (count);
                    return table.ok ? juce::String() : damaged;
                }
            }

            if (kitFirst)
                table.skip (pathFileBytes);

            if (! table.ok)
                return damaged;

            paths.push_back (std::move (p));
        }

        return {};
    }

    juce::String readRecords (In records, Kit& kit)
    {
        std::vector<Cell> cells;
        bool program = false, pathTable = false, kitFirst = false;
        In zones;

        while (records.left() > 0)
        {
            const auto id = (int) records.u16();
            auto body = records.take (records.u32());

            if (! records.ok)
                return damaged;

            if (id == programRecord && ! program)
            {
                program = true;
                Parts parts;

                if (! readParts (body, parts))
                    return damaged;

                if (parts.version < oldestProgram || parts.version > newestProgram)
                    return unknownVersion ("program", parts.version);

                kit.programVersion = parts.version;
                bool hadCells = false, hadZones = false, hadGroups = false;
                auto& children = parts.children;

                while (children.left() > 0)
                {
                    const auto child = (int) children.u16();
                    auto childBody = children.take (children.u32());

                    if (! children.ok)
                        return damaged;

                    if (child == cellsRecord && ! hadCells)
                    {
                        hadCells = true;

                        if (const auto error = readCells (childBody, cells); error.isNotEmpty())
                            return error;
                    }
                    else if (child == zonesRecord && ! hadZones)
                    {
                        hadZones = true;
                        zones = childBody;
                    }
                    else if (child == voiceGroupsRecord && ! hadGroups)
                    {
                        hadGroups = true;
                        kit.voiceGroupsRead = readVoiceGroups (childBody, kit);
                    }
                }

                if (! hadCells)
                    return notAKit;

                // The zones name cells, so they are read after them wherever they stand.
                if (hadZones)
                    if (const auto error = readZones (zones, cells); error.isNotEmpty())
                        return error;
            }
            else if (id == pathsRecord && ! pathTable)
            {
                pathTable = true;

                if (const auto error = readPaths (body, kit.paths, kitFirst); error.isNotEmpty())
                    return error;
            }
        }

        if (! program)
            return notAKit;

        // Where the table began with the kit file, that is taken off and the samples count from 0.
        if (kitFirst && ! kit.paths.empty())
            kit.paths.erase (kit.paths.begin());

        for (auto& c : cells)
        {
            for (auto& l : c.layers)
            {
                if (kitFirst)
                    l.path = l.path >= 1 ? l.path - 1 : -1;

                if (! juce::isPositiveAndBelow (l.path, (int) kit.paths.size()))
                    l.path = -1;
            }

            // A cell may be in a group the kit does not keep, which is then as a group starts.
            if (! kit.voiceGroupsRead || ! juce::isPositiveAndBelow (c.voiceGroup, numVoiceGroups))
                c.voiceGroup = -1;
            else if (c.voiceGroup >= (int) kit.voiceGroups.size())
                kit.voiceGroups.resize ((size_t) c.voiceGroup + 1);

            if (c.name.isNotEmpty() || ! c.layers.empty())
                kit.cells.push_back (std::move (c));
        }

        return {};
    }

    //==============================================================================
    // Writing, for the tests.
    struct Out
    {
        std::vector<uint8> bytes;

        void u8 (int v)                               { bytes.push_back ((uint8) v); }
        void u16 (int v)                              { u8 (v & 0xff); u8 ((v >> 8) & 0xff); }
        void u32 (uint32 v)                           { u16 ((int) (v & 0xffff)); u16 ((int) (v >> 16)); }
        void i32 (int v)                              { u32 ((uint32) v); }
        void u64 (uint64 v)                           { u32 ((uint32) (v & 0xffffffffu)); u32 ((uint32) (v >> 32)); }
        void zeros (size_t n)                         { bytes.insert (bytes.end(), n, (uint8) 0); }
        void tag (const char* t)                      { bytes.insert (bytes.end(), t, t + 4); }
        void add (const Out& o)                       { bytes.insert (bytes.end(), o.bytes.begin(), o.bytes.end()); }
        void add (const void* d, size_t n)            { bytes.insert (bytes.end(), (const uint8*) d, (const uint8*) d + n); }

        void f32 (float v)
        {
            uint32 bits;
            std::memcpy (&bits, &v, sizeof (bits));
            u32 (bits);
        }

        void text (const juce::String& s)
        {
            const auto utf16 = s.toUTF16();
            const auto units = juce::CharPointer_UTF16::getBytesRequiredFor (s.getCharPointer()) / 2;
            u32 ((uint32) units);

            for (size_t i = 0; i < units; ++i)
                u16 ((int) (uint16) utf16.getAddress()[i]);
        }
    };

    Out record (int id, const Out& body)
    {
        Out o;
        o.u16 (id);
        o.u32 ((uint32) body.bytes.size());
        o.add (body);
        return o;
    }

    Out partsOf (int version, const Out& priv, const Out& pub, const Out& children)
    {
        Out o;
        o.u8 (1);
        o.u16 (version);

        for (auto* part : { &priv, &pub, &children })
        {
            o.u32 ((uint32) part->bytes.size());
            o.add (*part);
        }

        return o;
    }

    /** A frame around another (or the end frame, with nothing inside). */
    Out frame (const char* domain, uint32 id, const Out& own, const Out* inside = nullptr)
    {
        Out o;
        o.u64 ((uint64) frameHeaderBytes + (inside != nullptr ? inside->bytes.size() : 0) + own.bytes.size());
        o.tag (domain);
        o.u32 (id);
        o.u32 (1);

        if (inside != nullptr)
            o.add (*inside);

        o.add (own);
        return o;
    }

    Out endOfFrames()
    {
        Out one;
        one.u32 (1);
        return frame (dataDomain, endFrame, one);
    }

    struct Child
    {
        const char* domain;
        uint32 id;
        Out item;
    };

    Out item (const Out& frames, const std::vector<Child>& children)
    {
        Out rest;
        rest.add (frames);
        rest.u32 (1);
        rest.u32 ((uint32) children.size());

        for (size_t i = 0; i < children.size(); ++i)
        {
            rest.u32 ((uint32) i);
            rest.tag (children[i].domain);
            rest.u32 (children[i].id);
            rest.add (children[i].item);
        }

        Out o;
        o.u64 ((uint64) itemHeaderBytes + rest.bytes.size());
        o.u32 (1);
        o.tag (itemTag);
        o.u64 (1);
        o.zeros (16);
        o.add (rest);
        return o;
    }

    bool safePart (const juce::String& part)
    {
        return part.isNotEmpty() && part != "." && part != ".." && ! part.containsAnyOf ("/\\") && ! part.containsChar ((juce::juce_wchar) 0xfffd);
    }
}

//==============================================================================
bool unpack (const void* packed, size_t packedSize, juce::MemoryBlock& out, size_t size)
{
    if (size > mostUnpackedBytes)
        return false;

    out.setSize (size);

    if (packedSize == 0)
        return size == 0;

    const auto* in = (const uint8*) packed;
    auto* to = (uint8*) out.getData();
    size_t ip = 0, at = 0;

    // The first byte's top bits name the layout; what it starts is always bytes as they are.
    auto control = (size_t) (in[ip++] & 31);

    for (;;)
    {
        if (control >= 32)
        {
            // A back-reference: so many bytes again from so far back.
            auto length = (control >> 5) - 1;
            const auto high = (control & 31) << 8;

            if (length == 6)
            {
                for (;;)
                {
                    if (ip >= packedSize)
                        return false;

                    const auto more = (size_t) in[ip++];
                    length += more;

                    if (more != 255)
                        break;
                }
            }

            if (ip >= packedSize)
                return false;

            const auto low = (size_t) in[ip++];
            auto distance = high + low + 1;

            if (low == 255 && high == (size_t) (31 << 8))
            {
                if (packedSize - ip < 2)
                    return false;

                distance = (((size_t) in[ip] << 8) | (size_t) in[ip + 1]) + nearDistance + 1;
                ip += 2;
            }

            length += 3;

            if (distance > at || length > size - at)
                return false;

            // One byte at a time: the bytes copied may be the ones just written.
            for (size_t i = 0; i < length; ++i, ++at)
                to[at] = to[at - distance];
        }
        else
        {
            const auto length = control + 1;

            if (length > packedSize - ip || length > size - at)
                return false;

            std::memcpy (to + at, in + ip, length);
            ip += length;
            at += length;
        }

        if (ip >= packedSize)
            break;

        control = in[ip++];
    }

    return at == size;
}

juce::MemoryBlock pack (const void* data, size_t size, bool runs)
{
    const auto* in = (const uint8*) data;
    std::vector<uint8> out;
    std::vector<uint8> plain;

    const auto flush = [&]
    {
        for (size_t i = 0; i < plain.size(); i += 32)
        {
            const auto n = juce::jmin ((size_t) 32, plain.size() - i);
            out.push_back ((uint8) ((out.empty() ? 0x20 : 0) | (n - 1)));
            out.insert (out.end(), plain.begin() + (std::ptrdiff_t) i, plain.begin() + (std::ptrdiff_t) (i + n));
        }

        plain.clear();
    };

    for (size_t at = 0; at < size;)
    {
        size_t best = 0, bestDistance = 0;

        // A near, a middling and a far place to look back to.
        if (runs && at > 0)
        {
            for (auto distance : { (size_t) 1, (size_t) 300, (size_t) 9000 })
            {
                if (distance > at)
                    break;

                size_t n = 0;

                while (at + n < size && in[at + n] == in[at + n - distance])
                    ++n;

                if (n > best)
                {
                    best = n;
                    bestDistance = distance;
                }
            }
        }

        if (best < 3)
        {
            plain.push_back (in[at++]);
            continue;
        }

        flush();
        const auto far = bestDistance - 1 >= nearDistance;
        const auto back = far ? bestDistance - 1 - nearDistance : bestDistance - 1;
        const auto high = far ? (size_t) 31 : back >> 8;

        if (best < 9)
        {
            out.push_back ((uint8) (((best - 2) << 5) | high));
        }
        else
        {
            out.push_back ((uint8) ((7 << 5) | high));
            auto more = best - 9;

            for (; more >= 255; more -= 255)
                out.push_back (255);

            out.push_back ((uint8) more);
        }

        if (far)
        {
            out.push_back (255);
            out.push_back ((uint8) (back >> 8));
            out.push_back ((uint8) (back & 255));
        }
        else
        {
            out.push_back ((uint8) (back & 255));
        }

        at += best;
    }

    flush();
    return juce::MemoryBlock (out.data(), out.size());
}

//==============================================================================
juce::String read (const void* data, size_t size, Kit& kit)
{
    kit = {};

    if (data == nullptr || size < (size_t) itemHeaderBytes)
        return notAKit;

    if (size > mostFileBytes)
        return "The file is larger than any Battery kit (" + juce::File::descriptionOfSizeInBytes ((juce::int64) mostFileBytes) + " is the most this reads).";

    In file { (const uint8*) data, size, 0, true };
    In block;

    if (! findInItem (file, dataDomain, blockFrame, block, 0))
        return notAKit;

    block.skip (4);
    const auto packed = block.u8();
    juce::MemoryBlock unpacked;
    In inner;

    if (packed == 1)
    {
        const auto unpackedSize = block.u32(), packedSize = block.u32();

        if (! block.ok || packedSize != block.left())
            return damaged;

        if ((size_t) unpackedSize > mostUnpackedBytes)
            return "The kit unpacks to more than " + juce::File::descriptionOfSizeInBytes ((juce::int64) mostUnpackedBytes) + ", the most this reads.";

        if (! unpack (block.data + block.at, block.left(), unpacked, (size_t) unpackedSize))
            return damaged;

        inner = { (const uint8*) unpacked.getData(), unpacked.getSize(), 0, true };
    }
    else if (packed == 0 && block.ok)
    {
        inner = block.take (block.left());
    }
    else
    {
        return damaged;
    }

    In recordsFrameBody;

    if (! findInItem (inner, dataDomain, recordsFrame, recordsFrameBody, 0))
        return notAKit;

    recordsFrameBody.skip (12);
    const auto records = recordsFrameBody.take (recordsFrameBody.u64());

    if (! recordsFrameBody.ok)
        return damaged;

    Kit readKit;

    if (const auto error = readRecords (records, readKit); error.isNotEmpty())
        return error;

    kit = std::move (readKit);
    return {};
}

juce::String read (const juce::File& file, Kit& kit)
{
    kit = {};

    if (! file.existsAsFile())
        return file.getFileName() + " is not there.";

    if (file.getSize() > (juce::int64) mostFileBytes)
        return read (nullptr, mostFileBytes + 1, kit).replace ("The file", file.getFileName());

    juce::MemoryBlock data;

    if (! file.loadFileAsData (data))
        return file.getFileName() + " could not be read.";

    return read (data.getData(), data.getSize(), kit);
}

juce::File resolve (const Path& path, const juce::File& kitFolder)
{
    if (path.parts.isEmpty() || path.up < 0 || path.up > mostPathParts)
        return {};

    auto file = kitFolder;

    for (int i = 0; i < path.up; ++i)
        file = file.getParentDirectory();

    for (auto& part : path.parts)
    {
        if (! safePart (part))
            return {};

        file = file.getChildFile (part);
    }

    return file;
}

juce::File resolveUnder (const Path& path, const juce::File& folder)
{
    for (auto& part : path.parts)
        if (! safePart (part))
            return {};

    for (int from = 0; from < path.parts.size(); ++from)
    {
        auto file = folder;

        for (int i = from; i < path.parts.size(); ++i)
            file = file.getChildFile (path.parts[i]);

        if (file.existsAsFile())
            return file;
    }

    return {};
}

//==============================================================================
juce::MemoryBlock write (const Kit& kit, const WriteOptions& options)
{
    const auto numCells = juce::jlimit (0, mostCells, options.numCells);
    std::vector<const Cell*> byPlace ((size_t) numCells, nullptr);

    for (auto& c : kit.cells)
        if (juce::isPositiveAndBelow (c.index, numCells))
            byPlace[(size_t) c.index] = &c;

    Out cells, zones;
    cells.u32 ((uint32) numCells);
    uint32 numZones = 0;

    for (int i = 0; i < numCells; ++i)
    {
        const Cell blank;
        const auto& c = byPlace[(size_t) i] != nullptr ? *byPlace[(size_t) i] : blank;

        Out priv, pub;
        priv.zeros (cellColourAt);
        priv.i32 (c.colour);
        priv.i32 (c.lowKey);
        priv.i32 (c.highKey);
        pub.text (c.name);
        pub.f32 (c.gain);
        pub.f32 (c.pan);
        pub.f32 (c.tune);
        pub.u8 (c.keyTracking ? 1 : 0);
        pub.u8 (c.reverse ? 1 : 0);
        pub.zeros (8);
        pub.i32 (c.voiceGroup >= 0 ? c.voiceGroup + 1 : -1);
        pub.zeros (12);
        Out cellChildren;

        if (c.envelopeRead)
        {
            const auto shortName = [] (Out& o, const char* name)
            {
                o.u32 ((uint32) std::strlen (name));
                o.add (name, std::strlen (name));
            };

            Out values, holderPub, modulatorPrivate, modulators;
            values.u8 (0);
            values.u16 (0x11);

            for (auto v : { -0.332f, c.attack, c.decay, c.hold, 1050.0f, 0.25f })
                values.f32 (v);

            holderPub.u32 (0);
            modulatorPrivate.u32 (1);
            shortName (modulatorPrivate, volumeTarget);
            modulatorPrivate.f32 (1.0f);
            modulatorPrivate.zeros (5);
            shortName (modulatorPrivate, volumeEnvelopeName);
            modulatorPrivate.zeros (3);
            modulatorPrivate.u8 (c.envelopeOn ? 0 : 1);
            modulatorPrivate.u16 (1);
            modulatorPrivate.u32 (907);
            shortName (modulatorPrivate, "ENV_AHDSR");
            modulatorPrivate.u32 (2);

            const auto modulator = partsOf (0x81, modulatorPrivate, {}, record (envelopeHolderRecord, partsOf (0x90, {}, holderPub, record (envelopeRecord, values))));
            modulators.u8 (0);
            modulators.u16 (0x12);
            modulators.u8 (1);
            modulators.u16 (modulatorRecord);
            modulators.u32 ((uint32) modulator.bytes.size());
            modulators.add (modulator);
            modulators.zeros (11);
            cellChildren = record (modulatorsRecord, modulators);
        }

        cells.add (partsOf (options.cellVersion, priv, pub, cellChildren));

        for (auto& l : c.layers)
        {
            Out zonePriv, zonePub;
            zonePriv.zeros (83);
            zonePub.i32 (l.start);
            zonePub.i32 (-l.endBack);
            zonePub.i32 (-1);

            for (auto v : { l.lowVelocity, l.highVelocity, l.lowKey, l.highKey, l.fadeLow, l.fadeHigh, 0, 0, l.rootKey })
                zonePub.u16 (v);

            zonePub.f32 (l.gain);
            zonePub.f32 (l.pan);
            zonePub.f32 (l.tune);

            if (options.zoneVersion == zoneVersionWithMore)
                zonePub.zeros (6);

            zonePub.i32 (options.pathsVersion == pathsVersionWithKit && l.path >= 0 ? l.path + 1 : l.path);
            zonePub.i32 (3);
            zonePub.i32 (44100);
            zonePub.u8 (2);
            zonePub.i32 (l.frames);
            zonePub.i32 (0);
            zonePub.i32 (60);
            zonePub.f32 (1.0f);
            zonePub.u8 (0);
            zonePub.i32 (0);

            zones.u32 ((uint32) i);
            zones.add (partsOf (options.zoneVersion, zonePriv, zonePub, {}));
            ++numZones;
        }
    }

    Out zoneList;
    zoneList.u32 (numZones);
    zoneList.add (zones);

    Out groups;

    const auto group = [&] (const VoiceGroup& g)
    {
        groups.u8 (0);
        groups.u16 (options.voiceGroupVersion);
        groups.text (g.name);
        groups.u8 (1);
        groups.u8 (0);
        groups.u8 (1);
        groups.i32 (g.voices);
        groups.i32 (10);
        groups.i32 (g.exclusiveGroup);
    };

    group ({ "<Kit>", kit.kitVoices, -1 });
    std::array<uint8, numVoiceGroups / 8> kept {};

    for (size_t i = 0; i < kit.voiceGroups.size() && i < (size_t) numVoiceGroups; ++i)
        if (! kit.voiceGroups[i].isDefault())
            kept[i / 8] = (uint8) (kept[i / 8] | (1 << (i % 8)));

    groups.add (kept.data(), kept.size());

    for (size_t i = 0; i < kit.voiceGroups.size() && i < (size_t) numVoiceGroups; ++i)
        if (! kit.voiceGroups[i].isDefault())
            group (kit.voiceGroups[i]);

    Out paths;
    const auto kitFirst = options.pathsVersion == pathsVersionWithKit;
    paths.u16 (options.pathsVersion);

    if (kitFirst)
    {
        paths.u32 ((uint32) kit.paths.size() + 2);
        paths.u32 (0);
        paths.u32 (0);
        paths.u32 (1);
        paths.u8 (4);
        paths.text ("Made Up.nbkt");
        paths.zeros (pathFileBytes);
    }
    else
    {
        paths.u32 (1);
        paths.u32 (0);
        paths.u32 ((uint32) kit.paths.size());
    }

    for (auto& p : kit.paths)
    {
        paths.u32 ((uint32) (p.parts.isEmpty() ? 0 : p.up + p.parts.size()));

        if (p.parts.isEmpty())
        {
            if (kitFirst)
                paths.zeros (pathFileBytes);

            continue;
        }

        for (int i = 0; i < p.up; ++i)
            paths.u8 (3);

        for (int i = 0; i < p.parts.size(); ++i)
        {
            paths.u8 (i == p.parts.size() - 1 ? 4 : 2);
            paths.text (p.parts[i]);
        }

        if (kitFirst)
            paths.zeros (pathFileBytes);
    }

    Out programChildren, programPrivate;
    programChildren.add (record (voiceGroupsRecord, groups));
    programChildren.add (record (cellsRecord, cells));
    programChildren.add (record (zonesRecord, zoneList));
    programPrivate.zeros (93);

    Out records;
    records.add (record (programRecord, partsOf (options.programVersion, programPrivate, {}, programChildren)));
    records.add (record (pathsRecord, paths));

    Out recordsBody;
    recordsBody.u32 (1);
    recordsBody.u32 (1);
    recordsBody.u32 (1);
    recordsBody.u64 ((uint64) records.bytes.size());
    recordsBody.add (records);
    recordsBody.zeros (8);

    const auto end = endOfFrames();
    const auto inner = item (end, { { dataDomain, recordsFrame, item (frame (dataDomain, recordsFrame, recordsBody, &end), {}) } });

    Out block;
    block.u32 (1);
    block.u8 (options.packed ? 1 : 0);

    if (options.packed)
    {
        const auto packed = pack (inner.bytes.data(), inner.bytes.size());
        block.u32 ((uint32) inner.bytes.size());
        block.u32 ((uint32) packed.getSize());
        block.add (packed.getData(), packed.getSize());
    }
    else
    {
        block.add (inner);
    }

    Out version;
    version.zeros (2);
    const auto blockFrames = frame (dataDomain, blockFrame, block, &end);
    const auto blockItem = item (frame (dataDomain, 116, {}, &blockFrames), {});
    const auto preset = item (frame (kontaktDomain, 3, version, &end), { { dataDomain, 116, blockItem } });
    const auto whole = item (end, { { kontaktDomain, 3, preset } });
    return juce::MemoryBlock (whole.bytes.data(), whole.bytes.size());
}
}
