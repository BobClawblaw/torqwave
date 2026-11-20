#pragma once

#include "Session.h"

/** Track templates (.sbtrack): a track's or bus's settings to start new ones from in any
    session. A template holds its name, whether it is a bus, its volume, pan, surround pan, polarity, colour, input,
    note, sends and output (by the bus's name, as ids mean nothing in another session), its effects
    chain, and its envelopes (volume, pan, surround pan and effect parameters, at the same times in
    the session it is added to, moved to its sample rate); not its clips, group, mute or solo.

    The same file is a channel-strip preset (apply): its effects, sends and settings put onto a
    track that is already there. */
namespace TrackTemplate
{
    inline constexpr const char* extension = ".sbtrack";

    /** The template of the track with this id (null when there is none), with its effects as
        TrackEffects::createXml gives them (or none). */
    std::unique_ptr<juce::XmlElement> create (const SessionState&, juce::uint32 trackId, std::unique_ptr<juce::XmlElement> effects);

    struct Added
    {
        juce::uint32 trackId = 0;                     // 0 when the XML is not a template
        std::shared_ptr<juce::XmlElement> effects;    // to load onto it, or null
        juce::StringArray missingBuses;               // sends and the output left out: no bus of that name here
    };

    /** Adds a track or bus made from a template at the end of the session. Sends go to the
        session's first bus of the same name, and so does the output. */
    Added add (SessionState&, const juce::XmlElement&);

    struct Applied
    {
        bool applied = false;                         // false when the XML is not a template, or the track can't take it
        std::shared_ptr<juce::XmlElement> effects;    // the chain to put in place of the track's (its slots without ids), or null to keep it
        bool keepInstrument = false;                  // the track's own instrument stays first, the effects after it
        juce::StringArray missingBuses;               // sends and the output left out: no bus of that name here
    };

    /** A channel-strip preset: gives a track (or bus, or a VCA fader its volume alone) a
        template's volume, pan, surround pan and polarity; a track also its sends, in place of
        its own, and both their output (a bus by name, an interface pair, or the mix). Its name,
        colour, note, input, channels, kind, clips, automation, group, mute and solo stay as
        they are. Effects come back to be put in place of its chain: on an instrument track a
        template's instrument replaces the track's, and with none the track keeps its own; on
        an audio track or a bus a template's instrument is left out. */
    Applied apply (SessionState&, juce::uint32 trackId, const juce::XmlElement&);
}

/** Channel-strip presets saved by name: track templates in a folder of their own. */
namespace ChannelStrips
{
    class Store
    {
    public:
        explicit Store (juce::File folder);

        /** The saved presets' names, sorted. */
        juce::StringArray getNames() const;
        juce::File fileFor (const juce::String& name) const;

        /** Writes a preset under a name (replacing one of that name); false when it can't. */
        bool save (const juce::String& name, const juce::XmlElement& strip) const;
        std::unique_ptr<juce::XmlElement> load (const juce::String& name) const;
        bool remove (const juce::String& name) const;

        juce::File getFolder() const    { return folder; }

    private:
        juce::File folder;
    };

    /** The user's presets, in the app's data folder. */
    Store& userStore();
}
