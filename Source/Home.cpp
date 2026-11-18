#include "Home.h"
#include "AudioFileIO.h"
#include "PreviousName.h"
#include "Project.h"
#include "RecentFiles.h"
#include "Theme.h"
#include "TrackColours.h"

namespace Home
{
Kind kindOf (const juce::File& f)
{
    return RecentFiles::isSession (f) ? Kind::session : Project::isProject (f) ? Kind::project : Kind::audio;
}

std::optional<Arrangement> arrangementOf (const juce::File& session)
{
    const auto xml = juce::parseXML (session);

    if (xml == nullptr || ! PreviousName::hasTag (*xml, "TORQWAVE_SESSION"))
        return std::nullopt;

    const auto rate = juce::jmax (1.0, xml->getDoubleAttribute ("sampleRate", 48000.0));
    Arrangement a;

    for (auto* trackXml : xml->getChildWithTagNameIterator ("TRACK"))
    {
        if (trackXml->getBoolAttribute ("bus") || trackXml->getBoolAttribute ("folder") || trackXml->getBoolAttribute ("hidden"))
            continue;

        Arrangement::Lane lane;
        lane.colour = trackXml->getIntAttribute ("colour", -1);

        for (const juce::String tag : { "CLIP", "NOTECLIP" })
            for (auto* clipXml : trackXml->getChildWithTagNameIterator (tag))
            {
                Arrangement::Clip c;
                c.start = (double) clipXml->getStringAttribute ("position").getLargeIntValue() / rate;
                c.end = c.start + (double) clipXml->getStringAttribute ("length").getLargeIntValue() / rate;
                c.colour = clipXml->getIntAttribute ("colour", -1);
                c.notes = tag == "NOTECLIP";

                if (c.end > c.start)
                {
                    a.length = juce::jmax (a.length, c.end);
                    lane.clips.push_back (c);
                }
            }

        a.lanes.push_back (std::move (lane));
    }

    return a;
}

ProjectFace faceOf (const juce::File& project)
{
    const auto xml = juce::parseXML (project);
    ProjectFace face;

    if (xml == nullptr)
        return face;

    // Paths as the project keeps them: relative to its folder when they are inside it.
    const auto pathOf = [&project] (const juce::XmlElement& e)
    {
        if (const auto relative = e.getStringAttribute ("pathRelative"); relative.isNotEmpty())
            if (const auto f = project.getParentDirectory().getChildFile (relative); f.exists())
                return f;

        const auto full = e.getStringAttribute ("path");
        return full.isNotEmpty() && juce::File::isAbsolutePath (full) ? juce::File (full) : juce::File();
    };

    juce::Array<juce::File> files;

    if (auto* list = xml->getChildByName ("FILES"))
        for (auto* e : list->getChildWithTagNameIterator ("FILE"))
            files.add (pathOf (*e));

    const auto* session = xml->getChildByName ("SESSION");

    if (session != nullptr && (xml->getBoolAttribute ("multitrack") || files.isEmpty()))
        face.session = pathOf (*session);
    else if (! files.isEmpty())
        face.audio = files[juce::jlimit (0, files.size() - 1, xml->getIntAttribute ("active"))];

    return face;
}

//==============================================================================
Pictures::Pictures (juce::File cacheFolder) : juce::Thread ("Home pictures"), cache (std::move (cacheFolder))
{
    AudioFileIO::registerFormats (formats);
    startThread (juce::Thread::Priority::low);
}

Pictures::~Pictures()
{
    cancelPendingUpdate();
    signalThreadShouldExit();
    notify();
    stopThread (10000);
}

juce::String Pictures::stampOf (const juce::File& f)
{
    return juce::String (f.getSize()) + ":" + juce::String (f.getLastModificationTime().toMilliseconds());
}

juce::File Pictures::cacheFile (const juce::File& f) const
{
    return cache.getChildFile (juce::String::toHexString (f.getFullPathName().hashCode64()) + ".peaks");
}

const Pictures::Entry* Pictures::find (const juce::File& f)
{
    const auto stamp = stampOf (f);
    const juce::ScopedLock sl (lock);

    if (const auto found = entries.find (f.getFullPathName()); found != entries.end() && found->second.stamp == stamp)
        return &found->second;

    if (! queue.contains (f.getFullPathName()))
    {
        queue.add (f.getFullPathName());
        notify();
    }

    return nullptr;
}

std::shared_ptr<const std::vector<float>> Pictures::waveform (const juce::File& f)
{
    const auto* e = find (f);
    return e != nullptr ? e->waveform : nullptr;
}

std::shared_ptr<const Arrangement> Pictures::arrangement (const juce::File& f)
{
    const auto* e = find (f);
    return e != nullptr ? e->arrangement : nullptr;
}

std::vector<float> Pictures::readWaveform (const juce::File& f, juce::AudioFormatManager& formats)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (f));

    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->numChannels == 0)
        return {};

    std::vector<float> peaks;
    std::vector<juce::Range<float>> levels (reader->numChannels);
    const auto n = reader->lengthInSamples;

    for (int i = 0; i < columns; ++i)
    {
        const auto start = n * i / columns, end = n * (i + 1) / columns;
        auto low = 0.0f, high = 0.0f;

        if (end > start)
        {
            reader->readMaxLevels (start, end - start, levels.data(), (int) reader->numChannels);

            for (const auto& r : levels)
            {
                low = juce::jmin (low, r.getStart());
                high = juce::jmax (high, r.getEnd());
            }
        }

        peaks.push_back (juce::jlimit (-1.0f, 1.0f, low));
        peaks.push_back (juce::jlimit (-1.0f, 1.0f, high));
    }

    return peaks;
}

void Pictures::run()
{
    while (! threadShouldExit())
    {
        juce::File f;

        {
            const juce::ScopedLock sl (lock);

            if (! queue.isEmpty())
            {
                f = juce::File (queue[0]);
                queue.remove (0);
            }
        }

        if (f == juce::File())
        {
            wait (-1);
            continue;
        }

        Entry e;
        e.stamp = stampOf (f);
        const auto kind = kindOf (f);
        auto audio = kind == Kind::audio ? f : juce::File();
        auto session = kind == Kind::session ? f : juce::File();

        if (kind == Kind::project)
        {
            const auto face = faceOf (f);
            audio = face.audio;
            session = face.session;
        }

        if (session != juce::File())
        {
            if (auto a = arrangementOf (session))
                e.arrangement = std::make_shared<const Arrangement> (std::move (*a));
        }
        else if (audio.existsAsFile())
        {
            // A waveform kept on disk for this version of the file, or read now and kept.
            const auto kept = cacheFile (audio);
            const auto audioStamp = stampOf (audio);
            juce::FileInputStream in (kept);
            std::vector<float> peaks;

            if (in.openedOk() && in.readString() == audioStamp && in.readInt() == columns * 2)
            {
                peaks.resize ((size_t) columns * 2);

                for (auto& p : peaks)
                    p = in.readFloat();
            }
            else
            {
                peaks = readWaveform (audio, formats);

                if (! peaks.empty() && (cache.isDirectory() || cache.createDirectory()))
                {
                    juce::TemporaryFile temp (kept);

                    if (auto out = temp.getFile().createOutputStream())
                    {
                        out->writeString (audioStamp);
                        out->writeInt ((int) peaks.size());

                        for (auto p : peaks)
                            out->writeFloat (p);

                        out.reset();
                        temp.overwriteTargetFileWithTemporary();
                    }
                }
            }

            if (! peaks.empty())
                e.waveform = std::make_shared<const std::vector<float>> (std::move (peaks));
        }

        e.failed = e.waveform == nullptr && e.arrangement == nullptr;

        {
            const juce::ScopedLock sl (lock);
            entries[f.getFullPathName()] = std::move (e);
        }

        triggerAsyncUpdate();
    }
}

void Pictures::handleAsyncUpdate()
{
    if (onChange != nullptr)
        onChange();
}

//==============================================================================
void drawPicture (juce::Graphics& g, juce::Rectangle<float> area, const juce::File& f, Pictures& pictures)
{
    g.setColour (Theme::canvas);
    g.fillRoundedRectangle (area, Theme::radius);
    const auto inner = area.reduced (8.0f, 8.0f);

    if (const auto a = pictures.arrangement (f); a != nullptr && ! a->lanes.empty() && a->length > 0.0)
    {
        const auto shown = juce::jmin ((int) a->lanes.size(), 8);
        const auto laneHeight = inner.getHeight() / (float) shown;

        for (int l = 0; l < shown; ++l)
        {
            const auto& lane = a->lanes[(size_t) l];
            const auto row = juce::Rectangle<float> (inner.getX(), inner.getY() + (float) l * laneHeight, inner.getWidth(), laneHeight).reduced (0.0f, 1.5f);

            for (const auto& c : lane.clips)
            {
                const auto x0 = row.getX() + (float) (c.start / a->length) * row.getWidth();
                const auto x1 = row.getX() + (float) (c.end / a->length) * row.getWidth();
                const auto colourIndex = juce::isPositiveAndBelow (c.colour, (int) TrackColours::palette.size()) ? c.colour
                                       : juce::isPositiveAndBelow (lane.colour, (int) TrackColours::palette.size()) ? lane.colour
                                       : l % TrackColours::automaticCount;
                const auto colour = juce::Colour (TrackColours::palette[(size_t) colourIndex].argb);
                const auto box = juce::Rectangle<float> (x0, row.getY(), juce::jmax (2.0f, x1 - x0 - 1.0f), row.getHeight());
                g.setColour (colour.withAlpha (c.notes ? 0.55f : 0.8f));
                g.fillRoundedRectangle (box, 2.0f);
            }
        }

        return;
    }

    if (const auto w = pictures.waveform (f); w != nullptr && w->size() == (size_t) Pictures::columns * 2)
    {
        const auto mid = inner.getCentreY(), half = inner.getHeight() / 2.0f;
        const auto step = inner.getWidth() / (float) Pictures::columns;
        juce::Path shape;

        for (int i = 0; i < Pictures::columns; ++i)
        {
            const auto x = inner.getX() + ((float) i + 0.5f) * step;
            const auto low = (*w)[(size_t) i * 2], high = (*w)[(size_t) i * 2 + 1];
            const auto top = mid - juce::jmax (high, 0.004f) * half, bottom = mid - juce::jmin (low, -0.004f) * half;
            shape.addRectangle (x - step * 0.4f, top, step * 0.8f, bottom - top);
        }

        g.setColour (Theme::waveform);
        g.fillPath (shape);
        return;
    }

    // Nothing yet (or nothing to read): a plain mark for the kind.
    g.setColour (Theme::faintText);
    g.setFont (juce::FontOptions (13.0f));
    const auto kind = kindOf (f);
    g.drawText (kind == Kind::audio ? "Audio" : kind == Kind::session ? "Session" : "Project", area, juce::Justification::centred);
}
}
