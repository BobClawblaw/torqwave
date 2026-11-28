#include "MpcKit.h"

namespace MpcKit
{
namespace
{
    bool isSafeToken (const juce::String& s)
    {
        if (s.isEmpty() || s.length() > 512)
            return false;

        const auto norm = s.replaceCharacter ('\\', '/');

        if (norm.startsWithChar ('/') || (norm.length() >= 2 && norm[1] == ':'))
            return false;

        const auto parts = juce::StringArray::fromTokens (norm, "/", "");

        for (auto& p : parts)
            if (p.isEmpty() || p == "." || p == "..")
                return false;

        return ! parts.isEmpty();
    }

    float childFloat (const juce::XmlElement& el, const char* tag, float fallback)
    {
        if (auto* c = el.getChildByName (tag))
        {
            const auto t = c->getAllSubText().trim();

            if (t.isNotEmpty())
            {
                const float v = t.getFloatValue();
                return std::isfinite (v) ? v : fallback;
            }
        }

        return fallback;
    }

    int childInt (const juce::XmlElement& el, const char* tag, int fallback)
    {
        if (auto* c = el.getChildByName (tag))
        {
            const auto t = c->getAllSubText().trim();
            return t.isNotEmpty() ? t.getIntValue() : fallback;
        }

        return fallback;
    }

    bool childBool (const juce::XmlElement& el, const char* tag, bool fallback)
    {
        if (auto* c = el.getChildByName (tag))
        {
            const auto t = c->getAllSubText().trim().toLowerCase();
            if (t == "true" || t == "1")  return true;
            if (t == "false" || t == "0") return false;
        }

        return fallback;
    }

    juce::String childText (const juce::XmlElement& el, const char* tag)
    {
        if (auto* c = el.getChildByName (tag))
            return c->getAllSubText().trim();

        return {};
    }

    int mapMpcFilterType (int raw) noexcept
    {
        // MPC FilterType: 0 Off, 1..4 Low-pass variants, 5..6 Band-pass, 7..8 High-pass
        if (raw <= 0) return 0;
        if (raw <= 4) return 1;
        if (raw <= 6) return 2;
        if (raw <= 8) return 3;
        return 1;
    }
}

juce::String parse (const juce::String& xmlText, Program& prog)
{
    prog = {};

    if (xmlText.isEmpty())
        return "This is not an Akai MPC program (.xpm) file.";

    auto root = juce::XmlDocument::parse (xmlText);

    if (root == nullptr)
        return "This is not a valid Akai MPC program (.xpm) XML file.";

    const juce::XmlElement* programEl = root->hasTagName ("Program") ? root.get() : root->getChildByName ("Program");

    if (programEl == nullptr)
        return "This XML file does not contain an Akai MPC <Program>.";

    const auto typeAttr = programEl->getStringAttribute ("type").trim().toLowerCase();
    prog.isKeygroup = (typeAttr == "keygroup");
    prog.name = childText (*programEl, "ProgramName");

    // Pad -> MIDI note table from <ProgramPads-v2.10> or <PadNoteMap>
    std::map<int, int> padNoteMap;

    if (auto* noteMap = programEl->getChildByName ("PadNoteMap"))
    {
        for (auto* pm : noteMap->getChildWithTagNameIterator ("PadNote"))
        {
            const int num = pm->getIntAttribute ("number", 0);
            const int note = childInt (*pm, "Note", -1);

            if (num >= 1 && note >= 0 && note <= 127)
                padNoteMap[num] = note;
        }
    }

    // Pad mute/choke groups from <PadGroupMap>
    std::map<int, int> padGroupMap;

    if (auto* groupMap = programEl->getChildByName ("PadGroupMap"))
    {
        for (auto* pg : groupMap->getChildWithTagNameIterator ("PadGroup"))
        {
            const int num = pg->getIntAttribute ("number", 0);
            const int grp = childInt (*pg, "Group", 0);

            if (num >= 1 && grp > 0)
                padGroupMap[num] = grp;
        }
    }

    auto* instrumentsEl = programEl->getChildByName ("Instruments");

    if (instrumentsEl == nullptr)
        return "This Akai MPC program has no <Instruments> section.";

    for (auto* instEl : instrumentsEl->getChildWithTagNameIterator ("Instrument"))
    {
        if ((int) prog.instruments.size() >= mostInstruments)
            break;

        Instrument inst;
        inst.number = instEl->getIntAttribute ("number", (int) prog.instruments.size() + 1);
        inst.lowNote = juce::jlimit (0, 127, childInt (*instEl, "LowNote", padNoteMap.count (inst.number) ? padNoteMap[inst.number] : (35 + inst.number)));
        inst.highNote = juce::jlimit (inst.lowNote, 127, childInt (*instEl, "HighNote", inst.lowNote));

        if (! prog.isKeygroup && padNoteMap.count (inst.number))
            inst.lowNote = inst.highNote = padNoteMap[inst.number];

        inst.muteGroup = childInt (*instEl, "MuteGroup", padGroupMap.count (inst.number) ? padGroupMap[inst.number] : 0);
        inst.volume = juce::jlimit (0.0f, 2.0f, childFloat (*instEl, "Volume", 0.707946f));
        inst.pan = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "Pan", 0.5f));
        inst.tuneCoarse = juce::jlimit (-36.0f, 36.0f, childFloat (*instEl, "TuneCoarse", 0.0f));
        inst.tuneFine = juce::jlimit (-100.0f, 100.0f, childFloat (*instEl, "TuneFine", 0.0f));
        inst.filterType = mapMpcFilterType (childInt (*instEl, "FilterType", 0));
        inst.cutoff = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "Cutoff", 1.0f));
        inst.resonance = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "Resonance", 0.0f));

        if (instEl->getChildByName ("VolumeAttack") != nullptr || instEl->getChildByName ("VolumeDecay") != nullptr)
        {
            inst.hasEnvelope = true;
            inst.attack = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "VolumeAttack", 0.0f));
            inst.hold = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "VolumeHold", 0.0f));
            inst.decay = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "VolumeDecay", 0.5f));
            inst.sustain = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "VolumeSustain", 1.0f));
            inst.release = juce::jlimit (0.0f, 1.0f, childFloat (*instEl, "VolumeRelease", 0.05f));
        }

        if (auto* layersEl = instEl->getChildByName ("Layers"))
        {
            for (auto* layEl : layersEl->getChildWithTagNameIterator ("Layer"))
            {
                if ((int) inst.layers.size() >= mostLayers)
                    break;

                const auto sName = childText (*layEl, "SampleName");
                const auto sFile = childText (*layEl, "SampleFile");

                if (sName.isEmpty() && sFile.isEmpty())
                    continue;

                Layer l;
                l.sampleName = sName;
                l.sampleFile = sFile;
                l.lowVelocity = juce::jlimit (0, 127, childInt (*layEl, "VelStart", 0));
                l.highVelocity = juce::jlimit (l.lowVelocity, 127, childInt (*layEl, "VelEnd", 127));
                l.volume = juce::jlimit (0.0f, 2.0f, childFloat (*layEl, "Volume", 1.0f));
                l.pan = juce::jlimit (0.0f, 1.0f, childFloat (*layEl, "Pan", 0.5f));
                l.tuneCoarse = juce::jlimit (-36.0f, 36.0f, childFloat (*layEl, "Pitch", childFloat (*layEl, "TuneCoarse", 0.0f)));
                l.tuneFine = juce::jlimit (-100.0f, 100.0f, childFloat (*layEl, "TuneFine", 0.0f));
                l.rootNote = juce::jlimit (0, 127, childInt (*layEl, "RootNote", inst.lowNote));
                l.sliceStart = juce::jmax (0, childInt (*layEl, "SliceStart", 0));
                l.sliceEnd = juce::jmax (0, childInt (*layEl, "SliceEnd", 0));
                l.reverse = childBool (*layEl, "Direction", false);
                l.loop = childBool (*layEl, "SliceLoop", false) || childInt (*layEl, "SliceLoop", 0) > 0;
                l.loopStart = juce::jmax (0, childInt (*layEl, "SliceLoopStart", 0));
                l.loopEnd = juce::jmax (0, childInt (*layEl, "SliceLoopEnd", 0));
                inst.layers.push_back (std::move (l));
            }
        }

        if (! inst.layers.empty())
        {
            const auto first = inst.layers.front().sampleName.isNotEmpty()
                                   ? inst.layers.front().sampleName
                                   : juce::File (inst.layers.front().sampleFile).getFileNameWithoutExtension();
            inst.name = first.isNotEmpty() ? first : ("Pad " + juce::String (inst.number));
            prog.instruments.push_back (std::move (inst));
        }
    }

    if (prog.instruments.empty())
        return "This Akai MPC program has no sample layers.";

    return {};
}

juce::String read (const juce::File& file, Program& prog)
{
    prog = {};

    if (! file.existsAsFile())
        return "Could not open " + file.getFileName() + ".";

    if (file.getSize() <= 0 || (size_t) file.getSize() > mostFileBytes)
        return "This Akai MPC program file is empty or exceeds the size limit.";

    const auto err = parse (file.loadFileAsString(), prog);

    if (err.isEmpty() && prog.name.isEmpty())
        prog.name = file.getFileNameWithoutExtension();

    return err;
}

juce::File resolve (const Layer& layer, const juce::File& xpmFile, const juce::File& library)
{
    juce::StringArray candidates;

    auto addName = [&] (const juce::String& s)
    {
        if (! isSafeToken (s))
            return;

        const auto norm = s.replaceCharacter ('\\', '/');
        candidates.addIfNotAlreadyThere (norm);

        if (! norm.containsChar ('.'))
        {
            candidates.addIfNotAlreadyThere (norm + ".wav");
            candidates.addIfNotAlreadyThere (norm + ".WAV");
            candidates.addIfNotAlreadyThere (norm + ".aif");
            candidates.addIfNotAlreadyThere (norm + ".aiff");
            candidates.addIfNotAlreadyThere (norm + ".flac");
        }
    };

    if (layer.sampleFile.isNotEmpty()) addName (layer.sampleFile);
    if (layer.sampleName.isNotEmpty()) addName (layer.sampleName);

    if (candidates.isEmpty())
        return {};

    juce::Array<juce::File> roots;
    const auto dir = xpmFile.isDirectory() ? xpmFile : xpmFile.getParentDirectory();

    if (dir != juce::File())
    {
        roots.add (dir);
        roots.add (dir.getChildFile ("Samples"));
        const auto parent = dir.getParentDirectory();
        if (parent != dir)
        {
            roots.add (parent);
            roots.add (parent.getChildFile ("Samples"));
        }
    }

    if (library != juce::File() && library.isDirectory())
    {
        roots.add (library);
        roots.add (library.getChildFile ("Samples"));
    }

    for (auto& root : roots)
    {
        if (! root.isDirectory())
            continue;

        for (auto& cand : candidates)
        {
            auto cur = root;
            for (auto& part : juce::StringArray::fromTokens (cand, "/", ""))
                cur = cur.getChildFile (part);

            if (cur.existsAsFile())
                return cur;
        }
    }

    return dir.getChildFile (candidates[0]);
}

SamplerResult toSamplerZones (const Program& prog, const juce::File& xpmFile, const juce::File& library)
{
    SamplerResult res;

    for (auto& inst : prog.instruments)
    {
        for (auto& lay : inst.layers)
        {
            const auto file = resolve (lay, xpmFile, library);

            if (! file.existsAsFile())
            {
                ++res.missingSamples;
                continue;
            }

            juce::String why;
            const auto sample = Sampler::sharedSample (file, why);

            if (sample == nullptr)
            {
                ++res.missingSamples;
                continue;
            }

            Sampler::Zone z;
            z.sample = sample;
            z.file = file;
            z.name = inst.name;
            z.lowKey = inst.lowNote;
            z.highKey = inst.highNote;
            const float totalSemis = inst.tuneCoarse + lay.tuneCoarse + (inst.tuneFine + lay.tuneFine) * 0.01f;
            const int coarseShift = (int) std::round (totalSemis);
            const int baseRoot = lay.rootNote > 0 ? lay.rootNote : inst.lowNote;
            z.root = juce::jlimit (0, 127, baseRoot - coarseShift);
            z.tune = juce::jlimit (-100.0f, 100.0f, (totalSemis - (float) coarseShift) * 100.0f);
            z.lowVelocity = juce::jlimit (1, 127, lay.lowVelocity <= 0 ? 1 : lay.lowVelocity);
            z.highVelocity = juce::jlimit (z.lowVelocity, 127, lay.highVelocity <= 0 ? 127 : lay.highVelocity);
            const float gain = juce::jmax (1.0e-4f, (inst.volume / 0.707946f) * lay.volume);
            z.level = juce::jlimit (-48.0f, 12.0f, 20.0f * std::log10 (gain));
            z.pan = juce::jlimit (-100.0f, 100.0f, ((inst.pan - 0.5f) + (lay.pan - 0.5f)) * 200.0f);
            z.reverse = lay.reverse;
            const int numFrames = sample->getNumFrames();
            z.start = juce::jlimit (0, juce::jmax (0, numFrames - 1), lay.sliceStart);
            z.end = lay.sliceEnd > z.start && lay.sliceEnd <= numFrames ? lay.sliceEnd : numFrames;
            z.choke = juce::jlimit (0, 8, inst.muteGroup);

            if (lay.loop && lay.loopEnd > lay.loopStart && lay.loopEnd <= numFrames)
            {
                z.loop = 1;
                z.loopStart = lay.loopStart;
                z.loopEnd = lay.loopEnd;
            }

            res.zones.push_back (std::move (z));
        }
    }

    if (res.zones.empty())
        res.error = res.missingSamples > 0 ? "None of the MPC program's samples were found."
                                           : "This MPC program has no playable zones.";

    return res;
}

juce::String writeXml (const Program& prog)
{
    juce::XmlElement root ("MPCVObject");
    auto* p = root.createNewChildElement ("Program");
    p->setAttribute ("type", prog.isKeygroup ? "Keygroup" : "Drum");
    p->createNewChildElement ("ProgramName")->addTextElement (prog.name.isNotEmpty() ? prog.name : "MPC Program");

    auto* insts = p->createNewChildElement ("Instruments");

    for (auto& inst : prog.instruments)
    {
        auto* ie = insts->createNewChildElement ("Instrument");
        ie->setAttribute ("number", inst.number);
        ie->createNewChildElement ("LowNote")->addTextElement (juce::String (inst.lowNote));
        ie->createNewChildElement ("HighNote")->addTextElement (juce::String (inst.highNote));
        ie->createNewChildElement ("MuteGroup")->addTextElement (juce::String (inst.muteGroup));
        ie->createNewChildElement ("Volume")->addTextElement (juce::String (inst.volume, 4));
        ie->createNewChildElement ("Pan")->addTextElement (juce::String (inst.pan, 4));
        ie->createNewChildElement ("TuneCoarse")->addTextElement (juce::String (inst.tuneCoarse, 2));
        ie->createNewChildElement ("TuneFine")->addTextElement (juce::String (inst.tuneFine, 2));
        ie->createNewChildElement ("FilterType")->addTextElement (juce::String (inst.filterType));
        ie->createNewChildElement ("Cutoff")->addTextElement (juce::String (inst.cutoff, 4));
        ie->createNewChildElement ("Resonance")->addTextElement (juce::String (inst.resonance, 4));

        if (inst.hasEnvelope)
        {
            ie->createNewChildElement ("VolumeAttack")->addTextElement (juce::String (inst.attack, 4));
            ie->createNewChildElement ("VolumeHold")->addTextElement (juce::String (inst.hold, 4));
            ie->createNewChildElement ("VolumeDecay")->addTextElement (juce::String (inst.decay, 4));
            ie->createNewChildElement ("VolumeSustain")->addTextElement (juce::String (inst.sustain, 4));
            ie->createNewChildElement ("VolumeRelease")->addTextElement (juce::String (inst.release, 4));
        }

        auto* layers = ie->createNewChildElement ("Layers");
        int num = 1;

        for (auto& l : inst.layers)
        {
            auto* le = layers->createNewChildElement ("Layer");
            le->setAttribute ("number", num++);
            if (l.sampleName.isNotEmpty()) le->createNewChildElement ("SampleName")->addTextElement (l.sampleName);
            if (l.sampleFile.isNotEmpty()) le->createNewChildElement ("SampleFile")->addTextElement (l.sampleFile);
            le->createNewChildElement ("VelStart")->addTextElement (juce::String (l.lowVelocity));
            le->createNewChildElement ("VelEnd")->addTextElement (juce::String (l.highVelocity));
            le->createNewChildElement ("Volume")->addTextElement (juce::String (l.volume, 4));
            le->createNewChildElement ("Pan")->addTextElement (juce::String (l.pan, 4));
            le->createNewChildElement ("Pitch")->addTextElement (juce::String (l.tuneCoarse, 2));
            le->createNewChildElement ("RootNote")->addTextElement (juce::String (l.rootNote));
            le->createNewChildElement ("SliceStart")->addTextElement (juce::String (l.sliceStart));
            le->createNewChildElement ("SliceEnd")->addTextElement (juce::String (l.sliceEnd));
            le->createNewChildElement ("Direction")->addTextElement (l.reverse ? "True" : "False");
            if (l.loop)
            {
                le->createNewChildElement ("SliceLoop")->addTextElement ("1");
                le->createNewChildElement ("SliceLoopStart")->addTextElement (juce::String (l.loopStart));
                le->createNewChildElement ("SliceLoopEnd")->addTextElement (juce::String (l.loopEnd));
            }
        }
    }

    return root.toString();
}
}
