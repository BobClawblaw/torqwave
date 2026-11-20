#include "AiSession.h"

namespace
{
    juce::var object (std::initializer_list<std::pair<const char*, juce::var>> properties)
    {
        auto* o = new juce::DynamicObject();

        for (auto& [name, value] : properties)
            o->setProperty (name, value);

        return juce::var (o);
    }

    juce::var refused (const juce::String& error)    { return object ({ { "ok", false }, { "error", error } }); }

    double round3 (double v)                          { return std::round (v * 1000.0) / 1000.0; }
    double round1 (double v)                          { return std::round (v * 10.0) / 10.0; }

    // Shared pieces of the tools' parameter schemas.
    const char* trackParam = R"("track": { "description": "The track, bus or folder: its number from get_session, or its name." })";
    const char* clipParam = R"("clip": { "type": "integer", "description": "The clip's id from list_clips." })";

    const char* toolsJson = R"JSON([
  { "name": "get_session", "description": "The session: its length, tempo, markers and regions, and every track, bus and folder in order with its number, kind, folder, volume, pan, mute, solo, output, sends and how many clips it holds and over what time. Call this first.",
    "parameters": { "type": "object", "properties": {} } },
  { "name": "list_clips", "description": "A track's clips: each one's id, name, start and end in seconds, gain, fades and whether it is muted.",
    "parameters": { "type": "object", "properties": { TRACK }, "required": ["track"] } },
  { "name": "add_track", "description": "Add an audio track, an instrument track, a bus or a folder at the end of the list (or after a track). An instrument track starts with no instrument.",
    "parameters": { "type": "object", "properties": { "kind": { "type": "string", "enum": ["audio", "instrument", "bus", "folder"] }, "name": { "type": "string" },
                    "after": { "description": "Put it after this track: its number or name. Leave out for the end." } }, "required": ["kind"] } },
  { "name": "set_track", "description": "Change a track's, bus's or folder's name, volume (dB, -60 to +12), pan (-1 left to +1 right), mute or solo. Give only what changes.",
    "parameters": { "type": "object", "properties": { TRACK, "name": { "type": "string" }, "volume": { "type": "number" }, "pan": { "type": "number" },
                    "mute": { "type": "boolean" }, "solo": { "type": "boolean" } }, "required": ["track"] } },
  { "name": "remove_track", "description": "Remove a track, bus or folder with its clips. Removing a bus removes the sends to it; removing a folder leaves its tracks, out of it.",
    "parameters": { "type": "object", "properties": { TRACK }, "required": ["track"] } },
  { "name": "move_track", "description": "Move a track, bus or folder to another place in the list.",
    "parameters": { "type": "object", "properties": { TRACK, "to": { "type": "integer", "description": "Its new number, from 1." } }, "required": ["track", "to"] } },
  { "name": "set_folder", "description": "Put a track or bus in a folder, or take it out with folder \"none\". Folders don't go in folders.",
    "parameters": { "type": "object", "properties": { TRACK, "folder": { "description": "The folder's number or name, or \"none\"." } }, "required": ["track", "folder"] } },
  { "name": "set_output", "description": "Send a track's or bus's output into a bus in place of the mix, or back to the mix with bus \"mix\".",
    "parameters": { "type": "object", "properties": { TRACK, "bus": { "description": "The bus's number or name, or \"mix\"." } }, "required": ["track", "bus"] } },
  { "name": "set_send", "description": "Add or change a send from a track or bus to a bus, at a level in dB (-60 to +12), after the track's volume unless pre_fader. remove: true takes the send away.",
    "parameters": { "type": "object", "properties": { TRACK, "bus": { "description": "The bus's number or name." }, "level": { "type": "number" },
                    "pre_fader": { "type": "boolean" }, "remove": { "type": "boolean" } }, "required": ["track", "bus"] } },
  { "name": "move_clip", "description": "Move a clip to a new start time, and to another track if one is given.",
    "parameters": { "type": "object", "properties": { CLIP, "start": { "type": "number", "description": "Seconds." }, TRACK }, "required": ["clip"] } },
  { "name": "trim_clip", "description": "Set where a clip starts and ends in the session, in seconds, showing more or less of its audio, which stays where it is in time.",
    "parameters": { "type": "object", "properties": { CLIP, "start": { "type": "number" }, "end": { "type": "number" } }, "required": ["clip"] } },
  { "name": "split_clip", "description": "Split a clip in two at a time inside it, in seconds. The first part keeps the clip's id.",
    "parameters": { "type": "object", "properties": { CLIP, "at": { "type": "number" } }, "required": ["clip", "at"] } },
  { "name": "set_clip", "description": "Change a clip's gain (dB, -60 to +24), fade in and fade out (seconds), or mute it. Give only what changes.",
    "parameters": { "type": "object", "properties": { CLIP, "gain": { "type": "number" }, "fade_in": { "type": "number" }, "fade_out": { "type": "number" },
                    "mute": { "type": "boolean" } }, "required": ["clip"] } },
  { "name": "remove_clip", "description": "Remove a clip. The clips after it stay where they are.",
    "parameters": { "type": "object", "properties": { CLIP }, "required": ["clip"] } },
  { "name": "add_marker", "description": "Add a marker at a time, or a region from start to end, in seconds on the session's time.",
    "parameters": { "type": "object", "properties": { "start": { "type": "number" }, "end": { "type": "number" }, "name": { "type": "string" } }, "required": ["start"] } },
  { "name": "add_note_clip", "description": "Add a note clip to an instrument track, with its notes. Note times are seconds from the clip's start. With loop_length the notes repeat every that many seconds to the clip's end, so a one-bar pattern can fill many bars; notes must then start inside the first loop.",
    "parameters": { "type": "object", "properties": { TRACK, "start": { "type": "number", "description": "Seconds, where the clip starts in the session." },
                    "length": { "type": "number", "description": "Seconds the clip lasts." }, "loop_length": { "type": "number", "description": "Seconds; leave out to play the notes once." },
                    "name": { "type": "string" },
                    "notes": { "type": "array", "items": { "type": "object", "properties": { "pitch": { "type": "integer", "description": "MIDI note, 0 to 127: 60 is middle C, C4 (so C2 is 36)" },
                               "start": { "type": "number" }, "length": { "type": "number" }, "velocity": { "type": "integer", "description": "1 to 127; 100 when left out" } },
                               "required": ["pitch", "start", "length"] } } },
                    "required": ["track", "start", "length", "notes"] } },
  { "name": "set_instrument", "description": "Choose an instrument track's instrument: an instrument plug-in by its id or name from list_plugins with instruments: true, and one of its saved presets (a drum sampler's kit, say) from list_instrument_presets. It is loaded when the plan is applied. On a track that already plays that instrument, give the preset alone to change to it.",
    "parameters": { "type": "object", "properties": { TRACK, "plugin": { "type": "string" }, "preset": { "type": "string", "description": "A saved preset's name; leave out for the instrument's own start" } }, "required": ["track"] } },
  { "name": "list_instrument_presets", "description": "The presets the user has saved for an instrument (the plug-in by id or name, or the instrument a track plays). Many instruments, drum samplers above all, keep their kits and sounds in their own browsers: those can be chosen here only once the user has saved them with Save Instrument Preset.",
    "parameters": { "type": "object", "properties": { "plugin": { "type": "string" }, TRACK } } },
  { "name": "set_automation", "description": "Draw a track's or bus's volume or pan automation: points joined by straight lines, held before the first and after the last. Volume points are dB (-60 to +12), pan points -1 (left) to +1 (right). The points replace the parameter's automation; no points remove it, giving the parameter back to the track's setting.",
    "parameters": { "type": "object", "properties": { TRACK, "parameter": { "type": "string", "enum": ["volume", "pan"] },
      "points": { "type": "array", "items": { "type": "object", "properties": { "time": { "type": "number", "description": "seconds" }, "value": { "type": "number" } }, "required": ["time", "value"] } } },
      "required": ["track", "parameter", "points"] } },
  { "name": "add_track_effect", "description": "Add an effect plug-in, by its id or name from list_plugins, at the end of a track's or bus's effects (after its instrument on an instrument track). It is added when the plan is applied, with the plug-in's default settings; its settings can't be changed here.",
    "parameters": { "type": "object", "properties": { "plugin": { "type": "string" }, TRACK }, "required": ["plugin", "track"] } },
  { "name": "set_tempo", "description": "Set the session's tempo (20 to 400 beats a minute) and beats in a bar (1 to 16) from the start.",
    "parameters": { "type": "object", "properties": { "bpm": { "type": "number" }, "beats_per_bar": { "type": "integer" } }, "required": ["bpm"] } }
])JSON";
}

namespace Ai
{
    SessionTools::SessionTools (SessionState s, juce::String name, juce::StringArray& planSteps,
                                std::map<juce::uint32, juce::PluginDescription> trackInstruments, FindInstrument find, PresetsOf presets,
                                std::map<juce::uint32, juce::StringArray> trackEffects, FindInstrument findEffectPlugin)
        : state (std::move (s)), sessionName (std::move (name)), steps (planSteps),
          instruments (std::move (trackInstruments)), findInstrument (std::move (find)), presetsOf (std::move (presets)),
          effects (std::move (trackEffects)), findEffect (std::move (findEffectPlugin))
    {
    }

    std::vector<SessionTools::InstrumentLoad> SessionTools::getInstrumentLoads() const
    {
        std::vector<InstrumentLoad> kept;

        for (auto& l : loads)
            if (const auto* t = state.findTrack (l.trackId); t != nullptr && t->takesNoteClips())
                kept.push_back (l);

        return kept;
    }

    std::vector<SessionTools::EffectLoad> SessionTools::getEffectLoads() const
    {
        std::vector<EffectLoad> kept;

        for (auto& l : effectLoads)
            if (state.findTrack (l.trackId) != nullptr)
                kept.push_back (l);

        return kept;
    }

    juce::Array<juce::var> SessionTools::toolDefinitions()
    {
        const auto text = juce::String (toolsJson).replace ("TRACK", trackParam).replace ("CLIP", clipParam);
        juce::Array<juce::var> tools;

        const auto parsed = juce::JSON::parse (text);

        if (auto* list = parsed.getArray())
            for (auto& t : *list)
                tools.add (object ({ { "type", "function" }, { "function", t } }));

        jassert (tools.size() == 21);
        return tools;
    }

    juce::String SessionTools::instructions()
    {
        return "You are working on a multitrack session: tracks of audio clips, instrument tracks of note clips, buses that tracks send to "
               "or play into, and folders that gather tracks. Times are seconds from the session's start. Tracks are named by their "
               "number in get_session's list or by their name; numbers change when tracks are added, moved or removed, so call "
               "get_session again after that. Clips are named by their id from list_clips. Each change adds a step to the plan; "
               "nothing reaches the session until the user applies it. Notes go in note clips on instrument tracks; a beat lasts "
               "60 / tempo seconds. Drum instruments mostly follow General MIDI: kick 36, snare 38, clap 39, closed hi-hat 42, "
               "open hi-hat 46, crash 49, ride 51. An instrument's sounds and kits are the presets the user saved for it "
               "(list_instrument_presets); when a request names a sound that isn't among them, say which there are rather than "
               "guessing. A track's effects: get_session lists them, and add_track_effect adds one at the end "
               "of the chain with its default settings. What these tools can't do yet: an effect's or instrument's own settings, "
               "removing or reordering effects, automation of an effect's parameters, bouncing or freezing a track, recording, and changing the audio inside a clip. Volume and pan automation (set_automation) take over from the track's setting while they have points. Say so when a request needs them, rather "
               "than doing something else.";
    }

    std::optional<juce::var> SessionTools::run (const juce::String& tool, const juce::var& args)
    {
        juce::var result;

        if      (tool == "get_session")    result = getSession();
        else if (tool == "list_clips")     result = listClips (args);
        else if (tool == "add_track")      result = addTrack (args);
        else if (tool == "set_track")      result = setTrack (args);
        else if (tool == "remove_track")   result = removeTrack (args);
        else if (tool == "move_track")     result = moveTrack (args);
        else if (tool == "set_folder")     result = setFolder (args);
        else if (tool == "set_output")     result = setOutput (args);
        else if (tool == "set_send")       result = setSend (args);
        else if (tool == "move_clip")      result = moveClip (args);
        else if (tool == "trim_clip")      result = trimClip (args);
        else if (tool == "split_clip")     result = splitClip (args);
        else if (tool == "set_clip")       result = setClip (args);
        else if (tool == "remove_clip")    result = removeClip (args);
        else if (tool == "add_marker")     result = addMarker (args);
        else if (tool == "set_tempo")      result = setTempo (args);
        else if (tool == "set_automation") result = setAutomation (args);
        else if (tool == "add_track_effect") result = addTrackEffect (args);
        else if (tool == "add_note_clip")  result = addNoteClip (args);
        else if (tool == "set_instrument") result = setInstrument (args);
        else if (tool == "list_instrument_presets") result = listInstrumentPresets (args);
        else return std::nullopt;

        return result;
    }

    //==============================================================================
    juce::String SessionTools::seconds (juce::int64 frame) const
    {
        return juce::String ((double) frame / state.sampleRate, 3);
    }

    std::optional<juce::int64> SessionTools::frameOf (const juce::var& v) const
    {
        if (! (v.isDouble() || v.isInt() || v.isInt64()))
            return std::nullopt;

        const auto s = (double) v;

        if (! std::isfinite (s) || s < 0.0 || s > 24.0 * 3600.0)
            return std::nullopt;

        return (juce::int64) std::llround (s * state.sampleRate);
    }

    juce::String SessionTools::kindOf (const SessionTrack& t) const
    {
        return t.isFolder ? "folder" : t.isBus ? "bus" : t.isInstrument ? "instrument" : "audio";
    }

    juce::String SessionTools::nameOf (juce::uint32 trackId) const
    {
        const auto* t = state.findTrack (trackId);
        return t != nullptr ? t->name : juce::String();
    }

    std::optional<int> SessionTools::trackOf (const juce::var& which, juce::String& error) const
    {
        const auto count = (int) state.tracks.size();

        if (which.isInt() || which.isInt64() || which.isDouble())
        {
            const auto n = (int) which;

            if (n >= 1 && n <= count && juce::exactlyEqual ((double) n, (double) which))
                return n - 1;

            error = "no track number " + which.toString() + "; get_session lists " + juce::String (count);
            return std::nullopt;
        }

        const auto name = which.toString().trim();

        if (name.containsOnly ("0123456789") && name.isNotEmpty())
            return trackOf (name.getIntValue(), error);

        for (int i = 0; i < count; ++i)
            if (state.tracks[(size_t) i].name.trim().equalsIgnoreCase (name))
                return i;

        error = name.isEmpty() ? juce::String ("no track given") : "no track named " + name.quoted() + "; get_session lists them";
        return std::nullopt;
    }

    juce::var SessionTools::done (const juce::String& step)
    {
        steps.add (step);
        edited = true;
        return object ({ { "ok", true }, { "step", step } });
    }

    //==============================================================================
    juce::var SessionTools::getSession()
    {
        juce::Array<juce::var> tracks;

        for (size_t i = 0; i < state.tracks.size(); ++i)
        {
            const auto& t = state.tracks[i];
            auto entry = object ({ { "number", (int) i + 1 }, { "name", t.name }, { "kind", kindOf (t) } });
            auto* o = entry.getDynamicObject();

            if (t.folderId != 0)
                o->setProperty ("folder", nameOf (t.folderId));

            o->setProperty ("volume", round1 (t.gainDb));

            if (! t.isFolder)
                o->setProperty ("pan", round3 (t.pan));

            o->setProperty ("mute", t.mute);
            o->setProperty ("solo", t.solo);

            if (! t.isFolder)
                o->setProperty ("output", t.outputBusId != 0 ? nameOf (t.outputBusId) : juce::String ("mix"));

            if (! t.sends.empty())
            {
                juce::Array<juce::var> sends;

                for (auto& s : t.sends)
                    sends.add (object ({ { "bus", nameOf (s.busId) }, { "level", round1 (s.gainDb) }, { "pre_fader", s.preFader } }));

                o->setProperty ("sends", sends);
            }

            const auto clipCount = (int) (t.clips.size() + t.noteClips.size());

            if (t.takesClips() || clipCount > 0)
            {
                o->setProperty ("clips", clipCount);

                if (clipCount > 0)
                {
                    auto first = std::numeric_limits<juce::int64>::max(), last = (juce::int64) 0;

                    for (auto& c : t.clips)     { first = juce::jmin (first, c.position); last = juce::jmax (last, c.getEnd()); }
                    for (auto& c : t.noteClips) { first = juce::jmin (first, c.position); last = juce::jmax (last, c.getEnd()); }

                    o->setProperty ("from", seconds (first).getDoubleValue());
                    o->setProperty ("to", seconds (last).getDoubleValue());
                }
            }

            if (t.isInstrument)
            {
                const auto pending = std::find_if (loads.rbegin(), loads.rend(), [&t] (const InstrumentLoad& l) { return l.trackId == t.id; });
                const auto current = instruments.find (t.id);
                o->setProperty ("instrument", pending != loads.rend() ? juce::var (pending->plugin.name + (pending->preset.isNotEmpty() ? ", preset " + pending->preset.quoted() : juce::String())
                                                                                  + " (when the plan is applied)")
                                              : current != instruments.end() ? juce::var (current->second.name) : juce::var ("none"));
            }

            if (t.isFrozen())
                o->setProperty ("frozen", true);

            {
                juce::StringArray chain;

                if (const auto current = effects.find (t.id); current != effects.end())
                    chain = current->second;

                for (auto& l : effectLoads)
                    if (l.trackId == t.id)
                        chain.add (l.plugin.name + " (when the plan is applied)");

                if (! chain.isEmpty())
                    o->setProperty ("effects", chain.joinIntoString (", "));
            }

            juce::StringArray automated;

            for (const auto& [name, e] : { std::pair<const char*, const Envelope*> { "volume", &t.volumeEnvelope }, { "pan", &t.panEnvelope } })
                if (! e->isEmpty())
                    automated.add (juce::String (name) + " (" + juce::String ((int) e->points.size()) + " points, "
                                   + seconds (e->points.front().frame) + " to " + seconds (e->points.back().frame) + " s)");

            if (! t.effectEnvelopes.empty() || t.hasSurroundAutomation())
                automated.add ("effect or surround parameters");

            if (! automated.isEmpty())
                o->setProperty ("automation", automated.joinIntoString ("; "));

            tracks.add (entry);
        }

        juce::Array<juce::var> markers;

        for (auto& m : state.markers)
        {
            auto entry = object ({ { "name", m.name }, { "start", seconds (m.start).getDoubleValue() } });

            if (m.isRegion())
                entry.getDynamicObject()->setProperty ("end", seconds (m.getEnd()).getDoubleValue());

            markers.add (entry);
        }

        return object ({ { "name", sessionName }, { "sample_rate", state.sampleRate }, { "length", seconds (state.getLength()).getDoubleValue() },
                         { "tempo", state.tempo }, { "beats_per_bar", state.beatsPerBar }, { "tracks", tracks }, { "markers", markers } });
    }

    juce::var SessionTools::listClips (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];
        juce::Array<juce::var> clips;

        for (auto& c : t.clips)
            clips.add (object ({ { "id", (int) c.id }, { "name", c.name }, { "start", seconds (c.position).getDoubleValue() },
                                 { "end", seconds (c.getEnd()).getDoubleValue() }, { "gain", round1 (c.gainDb) },
                                 { "fade_in", seconds (c.fadeIn).getDoubleValue() }, { "fade_out", seconds (c.fadeOut).getDoubleValue() },
                                 { "muted", c.muted }, { "locked", c.locked } }));

        for (auto& c : t.noteClips)
            clips.add (object ({ { "id", (int) c.id }, { "name", c.name }, { "notes", (int) c.notes.size() },
                                 { "start", seconds (c.position).getDoubleValue() }, { "end", seconds (c.getEnd()).getDoubleValue() },
                                 { "loop_length", c.loopLength > 0 ? juce::var (seconds (c.loopLength).getDoubleValue()) : juce::var() },
                                 { "muted", c.muted } }));

        return object ({ { "track", t.name }, { "clips", clips } });
    }

    //==============================================================================
    juce::var SessionTools::addTrack (const juce::var& a)
    {
        const auto kind = a["kind"].toString();
        int after = (int) state.tracks.size() - 1;

        if (! a["after"].isVoid() && a["after"].toString().isNotEmpty())
        {
            juce::String error;
            const auto index = trackOf (a["after"], error);

            if (! index)
                return refused (error);

            after = *index;
        }

        SessionTrack* added = nullptr;

        if      (kind == "audio")       added = &state.addTrack();
        else if (kind == "instrument")  added = &state.addInstrumentTrack();
        else if (kind == "bus")         added = &state.addBus();
        else if (kind == "folder")      added = &state.addFolder();
        else return refused ("kind must be audio, instrument, bus or folder");

        if (const auto name = a["name"].toString().trim(); name.isNotEmpty())
            added->name = name;

        const auto name = added->name;
        const auto at = after + 1;

        if (at < (int) state.tracks.size() - 1)
            SessionEdits::moveTrack (state, (int) state.tracks.size() - 1, at);

        auto result = done ("Add " + (kind == "audio" ? juce::String ("track") : kind == "instrument" ? juce::String ("instrument track") : kind) + " " + name.quoted()
                            + (at < (int) state.tracks.size() - 1 ? " after " + state.tracks[(size_t) after].name.quoted() : juce::String()));
        result.getDynamicObject()->setProperty ("number", at + 1);
        return result;
    }

    juce::var SessionTools::setTrack (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        auto& t = state.tracks[(size_t) *index];
        const auto before = t;
        juce::StringArray what;

        if (const auto name = a["name"].toString().trim(); a.hasProperty ("name") && name.isNotEmpty() && name != t.name)
        {
            what.add ("name " + name.quoted());
            t.name = name;
        }

        if (a.hasProperty ("volume"))
        {
            const auto v = (double) a["volume"];

            if (! std::isfinite (v) || v < -60.0 || v > 12.0)
                return refused ("volume must be -60 to +12 dB");

            t.gainDb = (float) v;
            what.add ("volume " + juce::String (v, 1) + " dB");
        }

        if (a.hasProperty ("pan"))
        {
            const auto p = (double) a["pan"];

            if (t.isFolder)
                return refused ("a folder has no pan");

            if (! std::isfinite (p) || p < -1.0 || p > 1.0)
                return refused ("pan must be -1 (left) to +1 (right)");

            t.pan = (float) p;
            what.add ("pan " + juce::String (p, 2));
        }

        if (a.hasProperty ("mute"))
        {
            t.mute = (bool) a["mute"];
            what.add (t.mute ? "muted" : "not muted");
        }

        if (a.hasProperty ("solo"))
        {
            t.solo = (bool) a["solo"];
            what.add (t.solo ? "soloed" : "not soloed");
        }

        if (what.isEmpty())
            return refused ("nothing to change: give name, volume, pan, mute or solo");

        SessionEdits::followGroup (state, before);
        return done ("Set " + before.name.quoted() + ": " + what.joinIntoString (", "));
    }

    juce::var SessionTools::removeTrack (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto name = state.tracks[(size_t) *index].name;
        const auto kind = kindOf (state.tracks[(size_t) *index]);

        if (! SessionEdits::removeTrack (state, *index))
            return refused ("it couldn't be removed");

        return done ("Remove " + (kind == "audio" || kind == "instrument" ? juce::String ("track") : kind) + " " + name.quoted());
    }

    juce::var SessionTools::moveTrack (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto to = (int) a["to"];

        if (to < 1 || to > (int) state.tracks.size())
            return refused ("to must be 1 to " + juce::String ((int) state.tracks.size()));

        const auto name = state.tracks[(size_t) *index].name;

        if (to - 1 == *index || ! SessionEdits::moveTrack (state, *index, to - 1))
            return refused ("it is there already, or can't go there");

        return done ("Move " + name.quoted() + " to number " + juce::String (to));
    }

    juce::var SessionTools::setFolder (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];
        const auto name = t.name;
        const auto trackId = t.id;
        juce::uint32 folderId = 0;
        juce::String folderName;

        if (! a["folder"].toString().trim().equalsIgnoreCase ("none"))
        {
            const auto folder = trackOf (a["folder"], error);

            if (! folder)
                return refused (error);

            if (! state.tracks[(size_t) *folder].isFolder)
                return refused (state.tracks[(size_t) *folder].name.quoted() + " isn't a folder");

            folderId = state.tracks[(size_t) *folder].id;
            folderName = state.tracks[(size_t) *folder].name;
        }

        if (! SessionEdits::setTrackFolder (state, trackId, folderId))
            return refused (t.isFolder ? "folders don't go in folders" : "it is so already");

        return done (folderId != 0 ? "Put " + name.quoted() + " in folder " + folderName.quoted() : "Take " + name.quoted() + " out of its folder");
    }

    juce::var SessionTools::setOutput (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto name = state.tracks[(size_t) *index].name;
        const auto trackId = state.tracks[(size_t) *index].id;
        juce::uint32 busId = 0;
        juce::String busName ("the mix");

        if (! a["bus"].toString().trim().equalsIgnoreCase ("mix"))
        {
            const auto bus = trackOf (a["bus"], error);

            if (! bus)
                return refused (error);

            if (! state.tracks[(size_t) *bus].isBus)
                return refused (state.tracks[(size_t) *bus].name.quoted() + " isn't a bus");

            busId = state.tracks[(size_t) *bus].id;
            busName = state.tracks[(size_t) *bus].name.quoted();
        }

        if (! SessionEdits::setTrackOutput (state, trackId, busId))
            return refused ("refused: a folder has no output, a bus can't play into itself or into a bus that plays into it, or it is so already");

        return done ("Play " + name.quoted() + " into " + busName);
    }

    juce::var SessionTools::setSend (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto bus = trackOf (a["bus"], error);

        if (! bus)
            return refused (error);

        if (! state.tracks[(size_t) *bus].isBus)
            return refused (state.tracks[(size_t) *bus].name.quoted() + " isn't a bus");

        const auto name = state.tracks[(size_t) *index].name, busName = state.tracks[(size_t) *bus].name;
        const auto trackId = state.tracks[(size_t) *index].id, busId = state.tracks[(size_t) *bus].id;

        if ((bool) a["remove"])
        {
            if (! SessionEdits::removeSend (state, trackId, busId))
                return refused ("there is no such send");

            return done ("Remove the send from " + name.quoted() + " to " + busName.quoted());
        }

        SessionSend send;
        send.busId = busId;
        send.gainDb = a.hasProperty ("level") ? (float) (double) a["level"] : 0.0f;
        send.preFader = (bool) a["pre_fader"];

        if (! std::isfinite (send.gainDb) || send.gainDb < -60.0f || send.gainDb > 12.0f)
            return refused ("level must be -60 to +12 dB");

        if (! SessionEdits::setSend (state, trackId, send))
            return refused ("refused: a folder has no sends, a bus can't send into itself or into a bus that feeds it, or it is so already");

        return done ("Send " + name.quoted() + " to " + busName.quoted() + " at " + juce::String (send.gainDb, 1) + " dB" + (send.preFader ? ", before its volume" : ""));
    }

    //==============================================================================
    juce::var SessionTools::moveClip (const juce::var& a)
    {
        const auto clipId = (juce::uint32) (int) a["clip"];
        const auto where = state.findClip (clipId);

        if (! where)
            return refused ("no audio clip with id " + a["clip"].toString() + "; list_clips gives them");

        const auto& clip = state.tracks[(size_t) where->first].clips[(size_t) where->second];
        const auto name = clip.name;
        auto position = clip.position;
        auto track = where->first;

        if (a.hasProperty ("start"))
        {
            const auto f = frameOf (a["start"]);

            if (! f)
                return refused ("start must be a time in seconds, 0 or later");

            position = *f;
        }

        if (a.hasProperty ("track"))
        {
            juce::String error;
            const auto t = trackOf (a["track"], error);

            if (! t)
                return refused (error);

            if (! state.tracks[(size_t) *t].takesAudioClips())
                return refused (state.tracks[(size_t) *t].name.quoted() + " doesn't take audio clips");

            track = *t;
        }

        const auto trackName = state.tracks[(size_t) track].name;

        if (! SessionEdits::moveClip (state, clipId, track, position))
            return refused ("the clip couldn't move there (it may be locked, or there already)");

        return done ("Move clip " + name.quoted() + " to " + seconds (position) + " s on " + trackName.quoted());
    }

    juce::var SessionTools::trimClip (const juce::var& a)
    {
        const auto clipId = (juce::uint32) (int) a["clip"];
        const auto where = state.findClip (clipId);

        if (! where)
            return refused ("no audio clip with id " + a["clip"].toString());

        const auto& clip = state.tracks[(size_t) where->first].clips[(size_t) where->second];
        const auto name = clip.name;
        auto start = clip.position, end = clip.getEnd();

        if (a.hasProperty ("start"))
        {
            const auto f = frameOf (a["start"]);
            if (! f) return refused ("start must be a time in seconds");
            start = *f;
        }

        if (a.hasProperty ("end"))
        {
            const auto f = frameOf (a["end"]);
            if (! f) return refused ("end must be a time in seconds");
            end = *f;
        }

        if (end <= start)
            return refused ("end must be after start");

        if (! SessionEdits::trimClip (state, clipId, start, end))
            return refused ("the clip couldn't be trimmed so (it may be locked, or so already)");

        const auto& now = state.tracks[(size_t) where->first].clips[(size_t) where->second];
        return done ("Trim clip " + name.quoted() + " to " + seconds (now.position) + " s to " + seconds (now.getEnd()) + " s");
    }

    juce::var SessionTools::splitClip (const juce::var& a)
    {
        const auto clipId = (juce::uint32) (int) a["clip"];
        const auto where = state.findClip (clipId);
        const auto at = frameOf (a["at"]);

        if (! where)
            return refused ("no audio clip with id " + a["clip"].toString());

        if (! at)
            return refused ("at must be a time in seconds");

        const auto name = state.tracks[(size_t) where->first].clips[(size_t) where->second].name;

        if (! SessionEdits::splitClip (state, clipId, *at))
            return refused ("the time must be inside the clip, and the clip not locked");

        return done ("Split clip " + name.quoted() + " at " + seconds (*at) + " s");
    }

    juce::var SessionTools::setClip (const juce::var& a)
    {
        const auto clipId = (juce::uint32) (int) a["clip"];
        const auto where = state.findClip (clipId);

        if (! where)
            return refused ("no audio clip with id " + a["clip"].toString());

        const auto clip = state.tracks[(size_t) where->first].clips[(size_t) where->second];
        juce::StringArray what;

        if (a.hasProperty ("gain"))
        {
            const auto g = (double) a["gain"];

            if (! std::isfinite (g) || g < SessionClip::minGainDb || g > SessionClip::maxGainDb)
                return refused ("gain must be -60 to +24 dB");

            if (! SessionEdits::setClipGain (state, clipId, (float) g) && ! juce::exactlyEqual ((float) g, clip.gainDb))
                return refused ("the gain couldn't be set (the clip may be locked)");

            what.add ("gain " + juce::String (g, 1) + " dB");
        }

        if (a.hasProperty ("fade_in") || a.hasProperty ("fade_out"))
        {
            auto in = clip.fadeIn, out = clip.fadeOut;

            if (a.hasProperty ("fade_in"))
            {
                const auto f = frameOf (a["fade_in"]);
                if (! f) return refused ("fade_in must be seconds, 0 or more");
                in = *f;
            }

            if (a.hasProperty ("fade_out"))
            {
                const auto f = frameOf (a["fade_out"]);
                if (! f) return refused ("fade_out must be seconds, 0 or more");
                out = *f;
            }

            if (in + out > clip.length)
                return refused ("the fades together are longer than the clip (" + seconds (clip.length) + " s)");

            if (! SessionEdits::setClipFades (state, clipId, in, out) && (in != clip.fadeIn || out != clip.fadeOut))
                return refused ("the fades couldn't be set (the clip may be locked)");

            what.add ("fades " + seconds (in) + " s in, " + seconds (out) + " s out");
        }

        if (a.hasProperty ("mute"))
        {
            const auto mute = (bool) a["mute"];

            if (mute != clip.muted && ! SessionEdits::setClipMuted (state, clipId, mute))
                return refused ("the clip couldn't be muted (it may be locked)");

            what.add (mute ? "muted" : "not muted");
        }

        if (what.isEmpty())
            return refused ("nothing to change: give gain, fade_in, fade_out or mute");

        return done ("Set clip " + clip.name.quoted() + ": " + what.joinIntoString (", "));
    }

    juce::var SessionTools::removeClip (const juce::var& a)
    {
        const auto clipId = (juce::uint32) (int) a["clip"];
        const auto where = state.findClip (clipId);

        if (! where)
            return refused ("no audio clip with id " + a["clip"].toString());

        const auto name = state.tracks[(size_t) where->first].clips[(size_t) where->second].name;

        if (! SessionEdits::removeClip (state, clipId))
            return refused ("the clip couldn't be removed (it may be locked)");

        return done ("Remove clip " + name.quoted());
    }

    //==============================================================================
    juce::var SessionTools::addMarker (const juce::var& a)
    {
        const auto start = frameOf (a["start"]);

        if (! start)
            return refused ("start must be a time in seconds, 0 or later");

        Marker m;
        m.start = *start;
        m.name = a["name"].toString().trim();

        if (a.hasProperty ("end"))
        {
            const auto end = frameOf (a["end"]);

            if (! end || *end <= *start)
                return refused ("end must be a time after start");

            m.length = *end - *start;
        }

        if (m.name.isEmpty())
            m.name = m.isRegion() ? "Region " + juce::String ((int) state.markers.size() + 1) : "Marker " + juce::String ((int) state.markers.size() + 1);

        state.markers.push_back (m);
        MarkerEdits::sort (state.markers);
        return done ("Add " + juce::String (m.isRegion() ? "region " : "marker ") + m.name.quoted() + " at " + seconds (m.start) + " s"
                     + (m.isRegion() ? " to " + seconds (m.getEnd()) + " s" : juce::String()));
    }

    juce::var SessionTools::setTempo (const juce::var& a)
    {
        const auto bpm = (double) a["bpm"];
        const auto beats = a.hasProperty ("beats_per_bar") ? (int) a["beats_per_bar"] : state.beatsPerBar;

        if (! std::isfinite (bpm) || bpm < 20.0 || bpm > 400.0 || beats < 1 || beats > 16)
            return refused ("bpm must be 20 to 400 and beats_per_bar 1 to 16");

        if (! SessionEdits::setTempo (state, bpm, beats))
            return refused ("the tempo is so already");

        return done ("Set the tempo to " + juce::String (bpm, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " bpm, "
                     + juce::String (beats) + " beats a bar");
    }

    juce::var SessionTools::addTrackEffect (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (t.isFolder)
            return refused ("a folder has no effects: add them to its tracks or a bus");

        if (t.isFrozen())
            return refused (t.name.quoted() + " is frozen: its effects can't change until it is unfrozen");

        if (findEffect == nullptr)
            return refused ("no plug-ins are known here");

        const auto plugin = findEffect (a["plugin"].toString().trim(), error);

        if (! plugin)
            return refused (error);

        if (plugin->isInstrument)
            return refused (plugin->name + " is an instrument: an instrument track's instrument is chosen with set_instrument");

        effectLoads.push_back ({ t.id, *plugin });
        return done ("Add " + plugin->name + " to " + t.name.quoted() + "'s effects (when the plan is applied)");
    }

    juce::var SessionTools::setAutomation (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];
        const auto which = a["parameter"].toString();

        if (which != "volume" && which != "pan")
            return refused ("parameter must be volume or pan");

        if (t.isFolder)
            return refused ("a folder has no " + which + " to automate");

        const auto parameter = which == "volume" ? AutomationParameter::volume : AutomationParameter::pan;
        const auto range = automationRange (parameter);
        const auto* points = a["points"].getArray();

        if (points == nullptr)
            return refused ("points must be a list of {time, value}");

        Envelope e;

        for (auto& point : *points)
        {
            const auto frame = frameOf (point["time"]);
            const auto& value = point["value"];

            if (! frame || ! (value.isDouble() || value.isInt() || value.isInt64()))
                return refused ("each point needs a time (seconds) and a value");

            const auto v = (double) value;

            if (! std::isfinite (v) || v < range.getStart() || v > range.getEnd())
                return refused (which == "volume" ? "volume points must be -60 to +12 dB" : "pan points must be -1 to +1");

            e.points.push_back ({ *frame, (float) v });
        }

        std::stable_sort (e.points.begin(), e.points.end(), [] (auto& x, auto& y) { return x.frame < y.frame; });

        if (t.envelope (parameter) == e)
            return refused ("the " + which + " automation is so already");

        if (! SessionEdits::setEnvelope (state, t.id, parameter, std::move (e)))
            return refused ("the " + which + " automation couldn't be set");

        const auto count = (int) points->size();
        return done (count == 0 ? "Remove " + t.name.quoted() + "'s " + which + " automation"
                                : "Automate " + t.name.quoted() + "'s " + which + " with " + juce::String (count) + " point" + (count == 1 ? "" : "s"));
    }

    //==============================================================================
    juce::var SessionTools::addNoteClip (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (! t.takesNoteClips())
            return refused (t.name.quoted() + " is not an instrument track: notes go on one (add_track with kind \"instrument\")");

        const auto start = frameOf (a["start"]), length = frameOf (a["length"]);

        if (! start || ! length || *length <= 0)
            return refused ("start and length are seconds, the length more than 0");

        NoteClip clip;
        clip.name = a["name"].toString().trim();
        clip.position = *start;
        clip.length = *length;

        if (! a["loop_length"].isVoid())
        {
            const auto loop = frameOf (a["loop_length"]);

            if (! loop || *loop <= 0 || *loop > *length)
                return refused ("loop_length is seconds, more than 0 and no longer than the clip");

            clip.loopLength = *loop;
        }

        const auto* notes = a["notes"].getArray();

        if (notes == nullptr || notes->isEmpty() || notes->size() > 4096)
            return refused ("notes is a list of 1 to 4096 notes, each with pitch, start and length");

        const auto room = clip.loopLength > 0 ? clip.loopLength : clip.length;

        for (int i = 0; i < notes->size(); ++i)
        {
            const auto& n = (*notes)[i];
            const auto& pitch = n["pitch"];
            const auto noteStart = frameOf (n["start"]), noteLength = frameOf (n["length"]);
            const auto velocity = n["velocity"].isVoid() ? 100.0 : (double) n["velocity"];
            const auto which = "note " + juce::String (i + 1) + ": ";

            if (! (pitch.isInt() || pitch.isInt64() || pitch.isDouble()) || (int) pitch < 0 || (int) pitch > 127
                || ! juce::exactlyEqual ((double) (int) pitch, (double) pitch))
                return refused (which + "pitch is a whole number from 0 to 127");

            if (! noteStart || ! noteLength || *noteLength <= 0)
                return refused (which + "start and length are seconds from the clip's start, the length more than 0");

            if (*noteStart >= room)
                return refused (which + "starts at " + seconds (*noteStart) + " s, past the " + (clip.loopLength > 0 ? "loop's" : "clip's")
                                + " length of " + seconds (room) + " s, so it would never play");

            if (! std::isfinite (velocity) || velocity < 1.0 || velocity > 127.0)
                return refused (which + "velocity is 1 to 127");

            clip.notes.push_back ({ (int) pitch, *noteStart, *noteLength, (float) (velocity / 127.0) });
        }

        const auto count = (int) clip.notes.size();
        const auto id = SessionEdits::addNoteClip (state, t.id, std::move (clip));

        if (id == 0)
            return refused ("the clip couldn't be added to " + t.name.quoted());

        auto result = done ("Add a note clip to " + t.name.quoted() + ", " + seconds (*start) + " s to " + seconds (*start + *length) + " s, "
                            + juce::String (count) + (count == 1 ? " note" : " notes")
                            + (! a["loop_length"].isVoid() ? ", repeating every " + seconds (frameOf (a["loop_length"]).value_or (0)) + " s" : juce::String()));
        result.getDynamicObject()->setProperty ("clip", (int) id);
        return result;
    }

    std::optional<juce::PluginDescription> SessionTools::instrumentOf (juce::uint32 trackId) const
    {
        for (auto l = loads.rbegin(); l != loads.rend(); ++l)
            if (l->trackId == trackId)
                return l->plugin;

        if (const auto current = instruments.find (trackId); current != instruments.end())
            return current->second;

        return std::nullopt;
    }

    juce::StringArray SessionTools::presetsFor (const juce::PluginDescription& d) const
    {
        return presetsOf != nullptr ? presetsOf (d) : juce::StringArray();
    }

    juce::var SessionTools::setInstrument (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (! t.takesNoteClips())
            return refused (t.name.quoted() + " is not an instrument track (add_track with kind \"instrument\" makes one)");

        const auto wanted = a["plugin"].toString().trim();
        const auto preset = a["preset"].toString().trim();
        const auto playing = instrumentOf (t.id);
        const auto current = instruments.find (t.id);
        const auto alreadyLoaded = current != instruments.end();
        std::optional<juce::PluginDescription> plugin;

        if (wanted.isEmpty())
        {
            if (! playing)
                return refused (t.name.quoted() + " has no instrument yet: name the plug-in");

            plugin = playing;
        }
        else
        {
            if (findInstrument == nullptr)
                return refused ("no plug-ins are known here");

            plugin = findInstrument (wanted, error);

            if (! plugin)
                return refused (error);

            if (! plugin->isInstrument)
                return refused (plugin->name + " is an effect, not an instrument; list_plugins with instruments: true lists those");
        }

        if (alreadyLoaded && current->second.createIdentifierString() != plugin->createIdentifierString())
            return refused (t.name.quoted() + " already plays " + current->second.name + "; replacing an instrument isn't in these tools yet");

        if (alreadyLoaded && preset.isEmpty())
            return refused (t.name.quoted() + " already plays " + plugin->name + "; give a preset to change its sound");

        juce::String presetName;

        if (preset.isNotEmpty())
        {
            const auto saved = presetsFor (*plugin);

            for (auto& n : saved)
                if (n.equalsIgnoreCase (preset))
                    presetName = n;

            if (presetName.isEmpty())
                return refused (plugin->name + " has no saved preset called " + preset.quoted() + "; "
                                + (saved.isEmpty() ? juce::String ("none are saved for it (the user saves them with Save Instrument Preset)")
                                                   : "its presets are " + saved.joinIntoString (", ")));
        }

        // A later choice for the same track replaces the earlier one, in the plan too.
        for (auto& l : loads)
            if (l.trackId == t.id)
                steps.removeString (l.step);

        loads.erase (std::remove_if (loads.begin(), loads.end(), [&t] (const InstrumentLoad& l) { return l.trackId == t.id; }), loads.end());

        const auto step = alreadyLoaded ? "Load the preset " + presetName.quoted() + " on " + t.name.quoted() + "'s " + plugin->name
                                        : "Load " + plugin->name + (presetName.isNotEmpty() ? " with the preset " + presetName.quoted() : juce::String())
                                              + " on " + t.name.quoted();
        loads.push_back ({ t.id, *plugin, presetName, alreadyLoaded, step });
        steps.add (step);
        return object ({ { "ok", true }, { "step", step } });
    }

    juce::var SessionTools::listInstrumentPresets (const juce::var& a)
    {
        juce::String error;
        std::optional<juce::PluginDescription> plugin;

        if (a["plugin"].toString().trim().isNotEmpty())
        {
            if (findInstrument == nullptr)
                return refused ("no plug-ins are known here");

            plugin = findInstrument (a["plugin"].toString().trim(), error);
        }
        else if (const auto index = trackOf (a["track"], error))
        {
            plugin = instrumentOf (state.tracks[(size_t) *index].id);

            if (! plugin)
                error = state.tracks[(size_t) *index].name.quoted() + " has no instrument";
        }

        if (! plugin)
            return refused (error.isNotEmpty() ? error : juce::String ("give a plugin or a track"));

        juce::Array<juce::var> names;

        for (auto& n : presetsFor (*plugin))
            names.add (n);

        return object ({ { "ok", true }, { "plugin", plugin->name }, { "presets", names },
                         { "note", names.isEmpty() ? juce::var ("None are saved for it. The user can load a sound in the instrument and save it with Save Instrument Preset.")
                                                   : juce::var() } });
    }
}
