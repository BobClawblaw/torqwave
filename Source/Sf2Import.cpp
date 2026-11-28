#include "Sf2Import.h"

namespace Sf2Import
{
namespace
{
    using juce::uint8;
    using juce::uint16;
    using juce::uint32;
    using juce::int16;
    using juce::int32;

    uint16 rdU16 (const uint8* d, size_t o) noexcept
    {
        return (uint16) (d[o] | (d[o + 1] << 8));
    }

    int16 rdI16 (const uint8* d, size_t o) noexcept
    {
        return (int16) rdU16 (d, o);
    }

    uint32 rdU32 (const uint8* d, size_t o) noexcept
    {
        return (uint32) d[o] | ((uint32) d[o + 1] << 8) | ((uint32) d[o + 2] << 16) | ((uint32) d[o + 3] << 24);
    }

    juce::String rdZStr (const uint8* d, size_t o, size_t maxLen)
    {
        size_t n = 0;
        while (n < maxLen && d[o + n] != 0)
            ++n;
        return juce::String::fromUTF8 ((const char*) (d + o), (int) n).trim();
    }

    struct Chunk
    {
        const uint8* data = nullptr;
        size_t size = 0;
    };

    // SF2 Generator IDs (SoundFont 2.01 specification section 8.1.2)
    enum GenId
    {
        genStartAddrsOffset = 0,
        genEndAddrsOffset = 1,
        genStartloopAddrsOffset = 2,
        genEndloopAddrsOffset = 3,
        genStartAddrsCoarseOffset = 4,
        genEndAddrsCoarseOffset = 12,
        genPan = 17,
        genInstrument = 41,
        genKeyRange = 43,
        genVelRange = 44,
        genStartloopAddrsCoarseOffset = 45,
        genInitialAttenuation = 48,
        genEndloopAddrsCoarseOffset = 50,
        genCoarseTune = 51,
        genFineTune = 52,
        genSampleID = 53,
        genSampleModes = 54,
        genOverridingRootKey = 58
    };

    struct ZoneGens
    {
        int lowKey = 0, highKey = 127;
        int lowVel = 0, highVel = 127;
        int overridingRoot = -1;
        int coarseTune = 0, fineTune = 0;
        int pan = 0;             // -500..+500 (tenths of a percent)
        int attenuationCb = 0;   // centibels (0..1440)
        int sampleModes = 0;     // 0 no loop, 1 continuous, 3 loop during sustain
        int startOff = 0, endOff = 0, loopStartOff = 0, loopEndOff = 0;
        int instrumentId = -1;
        int sampleId = -1;
    };

    void applyGen (ZoneGens& g, uint16 op, uint16 raw) noexcept
    {
        const int16 s = (int16) raw;
        switch (op)
        {
            case genStartAddrsOffset:           g.startOff += s; break;
            case genEndAddrsOffset:             g.endOff += s; break;
            case genStartloopAddrsOffset:       g.loopStartOff += s; break;
            case genEndloopAddrsOffset:         g.loopEndOff += s; break;
            case genStartAddrsCoarseOffset:     g.startOff += (int) s * 32768; break;
            case genEndAddrsCoarseOffset:       g.endOff += (int) s * 32768; break;
            case genStartloopAddrsCoarseOffset: g.loopStartOff += (int) s * 32768; break;
            case genEndloopAddrsCoarseOffset:   g.loopEndOff += (int) s * 32768; break;
            case genPan:                        g.pan = s; break;
            case genInstrument:                 g.instrumentId = raw; break;
            case genKeyRange:                   g.lowKey = raw & 0xff; g.highKey = (raw >> 8) & 0xff; break;
            case genVelRange:                   g.lowVel = raw & 0xff; g.highVel = (raw >> 8) & 0xff; break;
            case genInitialAttenuation:         g.attenuationCb = juce::jmax (0, (int) s); break;
            case genCoarseTune:                 g.coarseTune = s; break;
            case genFineTune:                   g.fineTune = s; break;
            case genSampleID:                   g.sampleId = raw; break;
            case genSampleModes:                g.sampleModes = raw & 3; break;
            case genOverridingRootKey:          if (s >= 0 && s <= 127) g.overridingRoot = s; break;
            default: break;
        }
    }

    struct Shdr
    {
        juce::String name;
        uint32 start = 0, end = 0, loopStart = 0, loopEnd = 0, sampleRate = 44100;
        uint8 originalPitch = 60;
        int8_t pitchCorrection = 0;
        uint16 sampleLink = 0, sampleType = 1;
    };

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

    void appendStr20 (juce::MemoryBlock& b, const juce::String& s)
    {
        char buf[20] {};
        const auto utf8 = s.toRawUTF8();
        std::strncpy (buf, utf8, 19);
        b.append (buf, 20);
    }

    void appendSubChunk (juce::MemoryBlock& into, const char* id, const juce::MemoryBlock& body)
    {
        into.append (id, 4);
        appendU32 (into, (uint32) body.getSize());
        into.append (body.getData(), body.getSize());
        if ((body.getSize() & 1) != 0)
        {
            const uint8 z = 0;
            into.append (&z, 1);
        }
    }
}

Result parse (const void* data, size_t size, int presetIndex)
{
    Result res;

    if (data == nullptr || size < 24)
    {
        res.error = "This is not a SoundFont 2 (.sf2) file.";
        return res;
    }

    if (size > mostFileBytes)
    {
        res.error = "This SoundFont 2 file exceeds the size limit.";
        return res;
    }

    const auto* d = static_cast<const uint8*> (data);

    if (std::memcmp (d, "RIFF", 4) != 0 || std::memcmp (d + 8, "sfbk", 4) != 0)
    {
        res.error = "This is not a SoundFont 2 (.sf2) file.";
        return res;
    }

    const size_t riffEnd = std::min<size_t> (size, (size_t) rdU32 (d, 4) + 8);
    Chunk smpl, phdr, pbag, pgen, inst, ibag, igen, shdr;

    // Walk top-level LIST chunks (INFO, sdta, pdta)
    size_t pos = 12;

    while (pos + 8 <= riffEnd)
    {
        const uint32 csz = rdU32 (d, pos + 4);
        const size_t bodyOff = pos + 8;
        const size_t nextOff = bodyOff + (size_t) csz + (csz & 1u);

        if (bodyOff + csz > riffEnd)
            break;

        if (std::memcmp (d + pos, "LIST", 4) == 0 && csz >= 4)
        {
            const uint8* listType = d + bodyOff;
            size_t sp = bodyOff + 4;
            const size_t listEnd = bodyOff + csz;

            while (sp + 8 <= listEnd)
            {
                const uint32 ssz = rdU32 (d, sp + 4);
                const size_t sb = sp + 8;

                if (sb + ssz > listEnd)
                    break;

                if (std::memcmp (listType, "INFO", 4) == 0 && std::memcmp (d + sp, "INAM", 4) == 0)
                    res.bankName = rdZStr (d, sb, ssz);
                else if (std::memcmp (listType, "sdta", 4) == 0 && std::memcmp (d + sp, "smpl", 4) == 0)
                    smpl = { d + sb, ssz };
                else if (std::memcmp (listType, "pdta", 4) == 0)
                {
                    if (std::memcmp (d + sp, "phdr", 4) == 0)      phdr = { d + sb, ssz };
                    else if (std::memcmp (d + sp, "pbag", 4) == 0) pbag = { d + sb, ssz };
                    else if (std::memcmp (d + sp, "pgen", 4) == 0) pgen = { d + sb, ssz };
                    else if (std::memcmp (d + sp, "inst", 4) == 0) inst = { d + sb, ssz };
                    else if (std::memcmp (d + sp, "ibag", 4) == 0) ibag = { d + sb, ssz };
                    else if (std::memcmp (d + sp, "igen", 4) == 0) igen = { d + sb, ssz };
                    else if (std::memcmp (d + sp, "shdr", 4) == 0) shdr = { d + sb, ssz };
                }

                sp = sb + ssz + (ssz & 1u);
            }
        }

        if (nextOff <= pos)
            break;

        pos = nextOff;
    }

    if (smpl.data == nullptr || phdr.size < 76 || pbag.size < 4 || inst.size < 44 || ibag.size < 4 || shdr.size < 92)
    {
        res.error = "This SoundFont 2 file is incomplete or damaged.";
        return res;
    }

    const size_t numPhdr = phdr.size / 38;
    const size_t numPbag = pbag.size / 4;
    const size_t numPgen = pgen.size / 4;
    const size_t numInst = inst.size / 22;
    const size_t numIbag = ibag.size / 4;
    const size_t numIgen = igen.size / 4;
    const size_t numShdr = shdr.size / 46;
    const size_t totalPcmSamples = smpl.size / 2;

    if (numPhdr < 2 || numInst < 2 || numShdr < 2 || totalPcmSamples == 0)
    {
        res.error = "This SoundFont 2 file contains no playable presets.";
        return res;
    }

    // Parse preset list (last phdr is terminal "EOP")
    for (size_t i = 0; i + 1 < numPhdr; ++i)
    {
        const size_t o = i * 38;
        res.presets.push_back ({ (int) rdU16 (phdr.data, o + 22), (int) rdU16 (phdr.data, o + 20), rdZStr (phdr.data, o, 20) });
    }

    // Parse sample headers (last shdr is terminal "EOS")
    std::vector<Shdr> shdrs;
    shdrs.reserve (numShdr - 1);

    for (size_t i = 0; i + 1 < numShdr; ++i)
    {
        const size_t o = i * 46;
        Shdr s;
        s.name = rdZStr (shdr.data, o, 20);
        s.start = rdU32 (shdr.data, o + 20);
        s.end = rdU32 (shdr.data, o + 24);
        s.loopStart = rdU32 (shdr.data, o + 28);
        s.loopEnd = rdU32 (shdr.data, o + 32);
        s.sampleRate = rdU32 (shdr.data, o + 36);
        s.originalPitch = shdr.data[o + 40];
        s.pitchCorrection = (int8_t) shdr.data[o + 41];
        s.sampleLink = rdU16 (shdr.data, o + 42);
        s.sampleType = rdU16 (shdr.data, o + 44);
        shdrs.push_back (std::move (s));
    }

    const size_t chosen = (size_t) juce::jlimit (0, (int) res.presets.size() - 1, presetIndex);
    res.presetName = res.presets[chosen].name;

    const size_t pBagStart = rdU16 (phdr.data, chosen * 38 + 24);
    const size_t pBagEnd = rdU16 (phdr.data, (chosen + 1) * 38 + 24);

    if (pBagStart >= pBagEnd || pBagEnd > numPbag)
    {
        res.error = "Selected SoundFont 2 preset has invalid zone indices.";
        return res;
    }

    ZoneGens pGlobal;

    for (size_t pb = pBagStart; pb < pBagEnd && (int) res.zones.size() < mostZones; ++pb)
    {
        const size_t pgStart = rdU16 (pbag.data, pb * 4);
        const size_t pgEnd = (pb + 1 < numPbag) ? rdU16 (pbag.data, (pb + 1) * 4) : numPgen;

        if (pgStart > pgEnd || pgEnd > numPgen)
            continue;

        ZoneGens pz = pGlobal;

        for (size_t pg = pgStart; pg < pgEnd; ++pg)
            applyGen (pz, rdU16 (pgen.data, pg * 4), rdU16 (pgen.data, pg * 4 + 2));

        // First preset bag without an instrument generator is the preset's global zone
        if (pz.instrumentId < 0)
        {
            if (pb == pBagStart)
                pGlobal = pz;
            continue;
        }

        if ((size_t) pz.instrumentId + 1 >= numInst)
            continue;

        const size_t iBagStart = rdU16 (inst.data, (size_t) pz.instrumentId * 22 + 20);
        const size_t iBagEnd = rdU16 (inst.data, ((size_t) pz.instrumentId + 1) * 22 + 20);

        if (iBagStart >= iBagEnd || iBagEnd > numIbag)
            continue;

        ZoneGens iGlobal;

        for (size_t ib = iBagStart; ib < iBagEnd && (int) res.zones.size() < mostZones; ++ib)
        {
            const size_t igStart = rdU16 (ibag.data, ib * 4);
            const size_t igEnd = (ib + 1 < numIbag) ? rdU16 (ibag.data, (ib + 1) * 4) : numIgen;

            if (igStart > igEnd || igEnd > numIgen)
                continue;

            ZoneGens iz = iGlobal;

            for (size_t ig = igStart; ig < igEnd; ++ig)
                applyGen (iz, rdU16 (igen.data, ig * 4), rdU16 (igen.data, ig * 4 + 2));

            if (iz.sampleId < 0)
            {
                if (ib == iBagStart)
                    iGlobal = iz;
                continue;
            }

            if ((size_t) iz.sampleId >= shdrs.size())
                continue;

            const auto& sh = shdrs[(size_t) iz.sampleId];

            // Skip right-channel stereo slave zones (we combine L+R when loading the left channel, or load mono)
            if (sh.sampleType == 2)
                continue;

            const int64_t sStart = std::clamp<int64_t> ((int64_t) sh.start + iz.startOff, 0, (int64_t) totalPcmSamples);
            const int64_t sEnd = std::clamp<int64_t> ((int64_t) sh.end + iz.endOff, sStart, (int64_t) totalPcmSamples);
            const int frames = (int) std::min<int64_t> (sEnd - sStart, (int64_t) (48000 * 60));

            if (frames < 8)
                continue;

            const bool hasRight = (sh.sampleType == 4 && sh.sampleLink < shdrs.size() && shdrs[sh.sampleLink].sampleType == 2);
            auto sample = std::make_shared<Sampler::Sample>();
            sample->rate = (double) std::clamp<uint32_t> (sh.sampleRate, 8000u, 192000u);
            sample->name = sh.name.isNotEmpty() ? sh.name : res.presetName;
            sample->audio.setSize (hasRight ? 2 : 1, frames);

            auto* dstL = sample->audio.getWritePointer (0);

            for (int f = 0; f < frames; ++f)
                dstL[f] = (float) rdI16 (smpl.data, (size_t) (sStart + f) * 2) * (1.0f / 32768.0f);

            if (hasRight)
            {
                const auto& shR = shdrs[sh.sampleLink];
                const int64_t rStart = std::clamp<int64_t> ((int64_t) shR.start + iz.startOff, 0, (int64_t) totalPcmSamples);
                const int64_t rAvail = std::max<int64_t> (0, (int64_t) totalPcmSamples - rStart);
                auto* dstR = sample->audio.getWritePointer (1);

                for (int f = 0; f < frames; ++f)
                    dstR[f] = f < rAvail ? (float) rdI16 (smpl.data, (size_t) (rStart + f) * 2) * (1.0f / 32768.0f) : dstL[f];
            }

            // Intersect preset and instrument key/velocity ranges
            const int lowKey = juce::jmax (pz.lowKey, iz.lowKey);
            const int highKey = juce::jmin (pz.highKey, iz.highKey);
            const int lowVel = juce::jlimit (1, 127, juce::jmax (1, juce::jmax (pz.lowVel, iz.lowVel)));
            const int highVel = juce::jlimit (lowVel, 127, juce::jmin (pz.highVel, iz.highVel));

            if (lowKey > highKey)
                continue;

            Sampler::Zone z;
            z.sample = std::move (sample);
            z.name = sh.name.isNotEmpty() ? sh.name : res.presetName;
            z.lowKey = lowKey;
            z.highKey = highKey;
            const int baseRoot = iz.overridingRoot >= 0 ? iz.overridingRoot
                                                        : (sh.originalPitch <= 127 ? (int) sh.originalPitch : 60);
            const float totalSemis = (float) (iz.coarseTune + pz.coarseTune)
                                   + (float) (iz.fineTune + pz.fineTune + sh.pitchCorrection) * 0.01f;
            const int coarseShift = (int) std::round (totalSemis);
            z.root = juce::jlimit (0, 127, baseRoot - coarseShift);
            z.tune = juce::jlimit (-100.0f, 100.0f, (totalSemis - (float) coarseShift) * 100.0f);
            z.lowVelocity = lowVel;
            z.highVelocity = highVel;
            z.start = 0;
            z.end = frames;
            z.pan = juce::jlimit (-100.0f, 100.0f, (float) (iz.pan + pz.pan) * 0.2f);
            z.level = juce::jlimit (-48.0f, 0.0f, -(float) (iz.attenuationCb + pz.attenuationCb) * 0.04f);

            if ((iz.sampleModes == 1 || iz.sampleModes == 3) && sh.loopEnd > sh.loopStart)
            {
                const int ls = (int) std::clamp<int64_t> ((int64_t) sh.loopStart + iz.loopStartOff - sStart, 0, frames - 1);
                const int le = (int) std::clamp<int64_t> ((int64_t) sh.loopEnd + iz.loopEndOff - sStart, ls + 1, frames);

                if (le - ls >= 16)
                {
                    z.loop = 1;
                    z.loopStart = ls;
                    z.loopEnd = le;
                    z.loopInRelease = (iz.sampleModes == 1);
                }
            }

            res.zones.push_back (std::move (z));
        }
    }

    if (res.zones.empty())
        res.error = "No playable zones were found in SoundFont preset \"" + res.presetName + "\".";

    return res;
}

Result loadFile (const juce::File& sf2File, int presetIndex)
{
    Result res;

    if (! sf2File.existsAsFile())
    {
        res.error = "Could not open " + sf2File.getFileName() + ".";
        return res;
    }

    if (sf2File.getSize() <= 0 || (size_t) sf2File.getSize() > mostFileBytes)
    {
        res.error = "SoundFont 2 file is empty or exceeds the size limit.";
        return res;
    }

    juce::MemoryBlock bytes;

    if (! sf2File.loadFileAsData (bytes))
    {
        res.error = "Could not read " + sf2File.getFileName() + ".";
        return res;
    }

    res = parse (bytes.getData(), bytes.getSize(), presetIndex);

    for (auto& z : res.zones)
        z.file = sf2File;

    return res;
}

juce::MemoryBlock writeMinimal (const juce::String& presetName, const std::vector<int16_t>& pcm,
                                int sampleRate, int rootKey, int lowKey, int highKey,
                                int lowVel, int highVel, bool looped, int loopStart, int loopEnd)
{
    // INFO LIST
    juce::MemoryBlock infoBody;
    infoBody.append ("INFO", 4);
    juce::MemoryBlock inam;
    inam.append (presetName.toRawUTF8(), (size_t) presetName.getNumBytesAsUTF8() + 1);
    appendSubChunk (infoBody, "INAM", inam);

    // sdta LIST -> smpl
    juce::MemoryBlock smplData;
    for (auto s : pcm)
        appendU16 (smplData, (uint16) s);
    // 46 zero guard samples required by SF2 spec
    for (int i = 0; i < 46; ++i)
        appendU16 (smplData, 0);

    juce::MemoryBlock sdtaBody;
    sdtaBody.append ("sdta", 4);
    appendSubChunk (sdtaBody, "smpl", smplData);

    // pdta LIST
    juce::MemoryBlock phdr, pbag, pgen, inst, ibag, igen, shdr;

    // 1 preset + EOP
    appendStr20 (phdr, presetName);
    appendU16 (phdr, 0); appendU16 (phdr, 0); appendU16 (phdr, 0);
    appendU32 (phdr, 0); appendU32 (phdr, 0); appendU32 (phdr, 0);
    appendStr20 (phdr, "EOP");
    appendU16 (phdr, 0); appendU16 (phdr, 0); appendU16 (phdr, 1);
    appendU32 (phdr, 0); appendU32 (phdr, 0); appendU32 (phdr, 0);

    appendU16 (pbag, 0); appendU16 (pbag, 0);
    appendU16 (pbag, 1); appendU16 (pbag, 0);

    appendU16 (pgen, genInstrument); appendU16 (pgen, 0);
    appendU16 (pgen, 0); appendU16 (pgen, 0);

    // 1 instrument + EOI
    appendStr20 (inst, presetName); appendU16 (inst, 0);
    appendStr20 (inst, "EOI"); appendU16 (inst, 1);

    appendU16 (ibag, 0); appendU16 (ibag, 0);
    const uint16 numIg = looped ? 5 : 4;
    appendU16 (ibag, numIg); appendU16 (ibag, 0);

    appendU16 (igen, genKeyRange); appendU16 (igen, (uint16) ((lowKey & 0xff) | ((highKey & 0xff) << 8)));
    appendU16 (igen, genVelRange); appendU16 (igen, (uint16) ((lowVel & 0xff) | ((highVel & 0xff) << 8)));
    appendU16 (igen, genOverridingRootKey); appendU16 (igen, (uint16) rootKey);
    if (looped)
    {
        appendU16 (igen, genSampleModes); appendU16 (igen, 1);
    }
    appendU16 (igen, genSampleID); appendU16 (igen, 0);
    appendU16 (igen, 0); appendU16 (igen, 0);

    // 1 sample + EOS
    appendStr20 (shdr, presetName);
    appendU32 (shdr, 0);
    appendU32 (shdr, (uint32) pcm.size());
    appendU32 (shdr, (uint32) loopStart);
    appendU32 (shdr, (uint32) loopEnd);
    appendU32 (shdr, (uint32) sampleRate);
    const uint8 pitchByte = (uint8) juce::jlimit (0, 127, rootKey);
    const uint8 corrByte = 0;
    shdr.append (&pitchByte, 1);
    shdr.append (&corrByte, 1);
    appendU16 (shdr, 0);
    appendU16 (shdr, 1); // mono

    appendStr20 (shdr, "EOS");
    for (int i = 0; i < 26; ++i)
    {
        const uint8 z = 0;
        shdr.append (&z, 1);
    }

    juce::MemoryBlock pdtaBody;
    pdtaBody.append ("pdta", 4);
    appendSubChunk (pdtaBody, "phdr", phdr);
    appendSubChunk (pdtaBody, "pbag", pbag);
    appendSubChunk (pdtaBody, "pgen", pgen);
    appendSubChunk (pdtaBody, "inst", inst);
    appendSubChunk (pdtaBody, "ibag", ibag);
    appendSubChunk (pdtaBody, "igen", igen);
    appendSubChunk (pdtaBody, "shdr", shdr);

    juce::MemoryBlock sfbk;
    sfbk.append ("sfbk", 4);
    appendSubChunk (sfbk, "LIST", infoBody);
    appendSubChunk (sfbk, "LIST", sdtaBody);
    appendSubChunk (sfbk, "LIST", pdtaBody);

    juce::MemoryBlock riff;
    riff.append ("RIFF", 4);
    appendU32 (riff, (uint32) sfbk.getSize());
    riff.append (sfbk.getData(), sfbk.getSize());
    return riff;
}
}
