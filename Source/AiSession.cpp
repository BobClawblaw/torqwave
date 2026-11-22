#include "AiSession.h"
#include "ClipEffects.h"
#include "Picture.h"

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
    const char* sectionParam = R"("section": { "description": "The section's name from get_session." })";
    const char* effectParam = R"("effect": { "description": "The plug-in in the track's chain: its number among the effects from get_session (from 1), its name, or \"instrument\" for an instrument track's instrument." })";

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
  { "name": "comp_takes", "description": "Build a comp from a clip's takes (its loop-recording passes, counted in list_clips): play take 'take' from start to end, in seconds, the rest as it was; joins are crossfaded. Without start and end, the take plays throughout and the comp ends.",
    "parameters": { "type": "object", "properties": { CLIP, "take": { "type": "integer", "description": "From 1." }, "start": { "type": "number" }, "end": { "type": "number" } },
                    "required": ["clip", "take"] } },
  { "name": "set_clip_effects", "description": "Change a clip's own effects, which change only that clip and none of its files: its pitch in semitones (-12 to +12, keeping its length; 0 for none), an effect plug-in to add at the end of its chain by its id or name from list_plugins (with the plug-in's default settings), or clear: true to take every effect off it. Give only what changes. The clip is rendered with them in the background.",
    "parameters": { "type": "object", "properties": { CLIP, "pitch": { "type": "number" }, "add": { "type": "string" }, "clear": { "type": "boolean" } }, "required": ["clip"] } },
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
  { "name": "set_automation", "description": "Draw automation of a track's or bus's volume or pan, or of a parameter of one of its effects or its instrument (give effect, and the parameter's name or id from list_effect_parameters): points joined by straight lines, held before the first and after the last. Volume points are dB (-60 to +12), pan points -1 (left) to +1 (right), an effect's parameter 0 to 1 (its whole range; list_effect_parameters says how the ends read). The points replace the parameter's automation; no points remove it, giving the parameter back to its setting. An effect added in this plan can be automated once the plan is applied.",
    "parameters": { "type": "object", "properties": { TRACK, "parameter": { "type": "string", "description": "volume, pan, or with effect the effect's parameter" }, FXREF,
      "points": { "type": "array", "items": { "type": "object", "properties": { "time": { "type": "number", "description": "seconds" }, "value": { "type": "number" } }, "required": ["time", "value"] } } },
      "required": ["track", "parameter", "points"] } },
  { "name": "add_track_effect", "description": "Add an effect plug-in, by its id or name from list_plugins, to a track's or bus's effects: at the end, or at a place among them (from 1). It is added when the plan is applied, with the plug-in's default settings: its settings and automation can be changed once it is there, in a later request.",
    "parameters": { "type": "object", "properties": { "plugin": { "type": "string" }, TRACK, "position": { "type": "integer", "description": "Its place among the effects, from 1; leave out for the end." } }, "required": ["plugin", "track"] } },
  { "name": "remove_track_effect", "description": "Take an effect off a track's or bus's chain, with any automation of its parameters. An instrument track's instrument stays.",
    "parameters": { "type": "object", "properties": { TRACK, FXREF }, "required": ["track", "effect"] } },
  { "name": "move_track_effect", "description": "Move an effect to another place in a track's or bus's chain (from 1, among the effects; the instrument stays first). The sound goes through the effects in order.",
    "parameters": { "type": "object", "properties": { TRACK, FXREF, "to": { "type": "integer" } }, "required": ["track", "effect", "to"] } },
  { "name": "list_effect_parameters", "description": "The parameters of an effect on a track, or of its instrument: each one's id, name, value (0 to 1) and how it reads, how its lowest and highest values read, and for a parameter of a few steps what each reads. find narrows the list to names containing it.",
    "parameters": { "type": "object", "properties": { TRACK, FXREF, "find": { "type": "string" } }, "required": ["track", "effect"] } },
  { "name": "set_track_effect", "description": "Change an effect's or an instrument's settings: switch an effect off (bypass true) or on, and set parameters, each by its name or id from list_effect_parameters, to a value from 0 to 1 or to text as the plug-in reads it (\"-6 dB\", \"Hall\", one of a stepped parameter's choices). Made when the plan is applied, not part of its undo step. A parameter with automation follows its automation instead.",
    "parameters": { "type": "object", "properties": { TRACK, FXREF, "bypass": { "type": "boolean" },
      "parameters": { "type": "array", "items": { "type": "object", "properties": { "parameter": { "type": "string" }, "value": { "type": "number" }, "text": { "type": "string" } }, "required": ["parameter"] } } },
      "required": ["track", "effect"] } },
  { "name": "freeze_track", "description": "Freeze a track (freeze true, the default): its clips are rendered through its effects and instrument, from the session's start to its last clip's end and a 5 s tail, and it plays that render instead, sparing the computer; its clips and effects can't change until it is unfrozen. Or unfreeze it (freeze false). Freezing renders once the plan is applied, with the rest of the plan in place, as its own undo step.",
    "parameters": { "type": "object", "properties": { TRACK, "freeze": { "type": "boolean" } }, "required": ["track"] } },
  { "name": "bounce_track", "description": "Bounce a track: render it as it goes to the mix, through its effects, volume, pan and automation, onto a new track right after it (with its sends, output and folder) and mute it. Over start to end in seconds, or the whole track when left out. It renders once the plan is applied, with the rest of the plan in place, as its own undo step; the new track isn't in this plan.",
    "parameters": { "type": "object", "properties": { TRACK, "start": { "type": "number" }, "end": { "type": "number" } }, "required": ["track"] } },
 { "name": "add_section", "description": "Add a section (Intro, Verse, Chorus...) to the arranger over a span of the session, in seconds; best on bar lines. Sections may not overlap.",
    "parameters": { "type": "object", "properties": { "name": { "type": "string" }, "start": { "type": "number" }, "end": { "type": "number" } }, "required": ["name", "start", "end"] } },
  { "name": "move_section", "description": "Move a section, with everything on every track in it (clips, notes, automation, markers, and its tempo when it is whole bars), so it starts at a time in seconds in the session as it will be: the gap it leaves closes, and room opens where it goes. With copy, a copy goes there and the section stays (\"double the chorus\": copy it to its own end).",
    "parameters": { "type": "object", "properties": { SECTION, "to": { "type": "number" }, "copy": { "type": "boolean" } }, "required": ["section", "to"] } },
  { "name": "delete_section", "description": "Delete a section and everything in its time; what comes after moves back to close the gap.",
    "parameters": { "type": "object", "properties": { SECTION }, "required": ["section"] } },
  { "name": "change_section_bars", "description": "Make a section longer or shorter by whole bars, at its start or its end: more bars put empty bars in (at the tempo there) and the rest of the session moves later; fewer take those bars and what is in them out, and the rest moves back.",
    "parameters": { "type": "object", "properties": { SECTION, "bars": { "type": "integer", "description": "Bars to add (positive) or take away (negative)." },
                    "at": { "type": "string", "enum": ["start", "end"] } }, "required": ["section", "bars"] } },
  { "name": "set_arrangement", "description": "Set the arrangement chain: the sections in the order to play them (a section may come more than once), and whether playback follows it. An empty list clears it.",
    "parameters": { "type": "object", "properties": { "sections": { "type": "array", "items": { "type": "string" }, "description": "Section names, in order." },
                    "play": { "type": "boolean" } }, "required": ["sections"] } },
  { "name": "set_tempo", "description": "Set the session's tempo (20 to 400 beats a minute) and beats in a bar (1 to 16) from the start.",
    "parameters": { "type": "object", "properties": { "bpm": { "type": "number" }, "beats_per_bar": { "type": "integer" } }, "required": ["bpm"] } },
  { "name": "fit_tempo_to_hits", "description": "Working to picture: find the tempo that puts hit markers (cuts and hits in the picture) on the beat, and set it. land_on: beats or bars give one tempo from the session's start, in place of any tempo changes; bar_lines gives a tempo map with each hit on the first beat of a bar, its tempo kept near the best fit. Looks between min_bpm and max_bpm (80 % to 125 % of the tempo now when left out). Returns the tempo and how far each hit lands from its beat or bar, in ms.",
    "parameters": { "type": "object", "properties": { "markers": { "type": "array", "items": { "type": "string" }, "description": "The point markers' names from get_session; leave out for every point marker." },
                    "land_on": { "type": "string", "enum": ["beats", "bars", "bar_lines"] }, "min_bpm": { "type": "number" }, "max_bpm": { "type": "number" } } } },
  { "name": "store_scene", "description": "Keep the mix as it is now (every track's and bus's volume, pan, mute, solo, polarity and sends) as a mixer scene, by a name. A scene of that name is replaced by the mix now. get_session lists the scenes.",
    "parameters": { "type": "object", "properties": { "name": { "type": "string" } }, "required": ["name"] } },
  { "name": "recall_scene", "description": "Set the mix back to a mixer scene's (by its name from get_session): the volume, pan, mute, solo, polarity and sends of every track it holds. Tracks added since keep theirs.",
    "parameters": { "type": "object", "properties": { "name": { "type": "string" } }, "required": ["name"] } }
])JSON";
}

namespace Ai
{
    SessionTools::SessionTools (SessionState s, juce::String name, juce::StringArray& planSteps,
                                std::map<juce::uint32, juce::PluginDescription> trackInstruments, FindInstrument find, PresetsOf presets,
                                Chains trackChains, FindInstrument findEffectPlugin)
        : state (std::move (s)), sessionName (std::move (name)), steps (planSteps),
          instruments (std::move (trackInstruments)), findInstrument (std::move (find)), presetsOf (std::move (presets)),
          chains (std::move (trackChains)), findEffect (std::move (findEffectPlugin))
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

    std::vector<SessionTools::ChainChange> SessionTools::getChainChanges() const
    {
        std::vector<ChainChange> kept;

        for (auto& t : state.tracks)
        {
            if (const auto p = planned.find (t.id); p != planned.end())
            {
                ChainChange change { t.id, {} };

                for (auto& slot : p->second)
                    change.chain.push_back (slot);

                kept.push_back (std::move (change));
            }
        }

        return kept;
    }

    std::vector<SessionTools::Render> SessionTools::getRenders() const
    {
        std::vector<Render> kept;

        for (auto& r : renders)
            if (state.findTrack (r.trackId) != nullptr)
                kept.push_back (r);

        return kept;
    }

    std::vector<SessionTools::PlannedSlot>& SessionTools::chainOf (juce::uint32 trackId)
    {
        if (const auto p = planned.find (trackId); p != planned.end())
            return p->second;

        return planned[trackId] = viewOf (trackId);
    }

    std::vector<SessionTools::PlannedSlot> SessionTools::viewOf (juce::uint32 trackId) const
    {
        if (const auto p = planned.find (trackId); p != planned.end())
            return p->second;

        std::vector<PlannedSlot> chain;

        if (const auto c = chains.find (trackId); c != chains.end())
        {
            for (auto& e : c->second)
            {
                PlannedSlot slot;
                slot.slotId = e.slotId;
                slot.plugin = e.plugin;
                slot.instrument = e.instrument;
                slot.bypassed = e.bypassed;
                slot.was = &e;
                chain.push_back (std::move (slot));
            }
        }

        return chain;
    }

    std::optional<size_t> SessionTools::effectOf (juce::uint32 trackId, const juce::var& which, juce::String& error) const
    {
        const auto chain = viewOf (trackId);
        const auto text = which.toString().trim();
        const auto& t = *state.findTrack (trackId);
        std::vector<size_t> effects;

        for (size_t i = 0; i < chain.size(); ++i)
            if (! chain[i].instrument)
                effects.push_back (i);

        if (text.isEmpty())
        {
            error = "say which effect: its number from get_session, its name, or \"instrument\"";
            return std::nullopt;
        }

        if (text.equalsIgnoreCase ("instrument"))
        {
            if (! chain.empty() && chain.front().instrument)
                return (size_t) 0;

            error = t.name.quoted() + " has no instrument loaded" + (t.isInstrument ? juce::String (" yet: one chosen in this plan is set once it is there") : juce::String());
            return std::nullopt;
        }

        if (which.isInt() || which.isInt64() || which.isDouble() || text.containsOnly ("0123456789"))
        {
            const auto n = text.getIntValue();

            if (text.isNotEmpty() && juce::isPositiveAndNotGreaterThan (n, (int) effects.size()))
                return effects[(size_t) n - 1];

            error = t.name.quoted() + " has " + (effects.empty() ? juce::String ("no effects") : juce::String ((int) effects.size()) + " effect" + (effects.size() == 1 ? "" : "s"))
                    + ": there is no effect " + text;
            return std::nullopt;
        }

        std::vector<size_t> named;

        for (size_t i = 0; i < chain.size(); ++i)
            if (chain[i].plugin.name.equalsIgnoreCase (text))
                named.push_back (i);

        if (named.size() == 1)
            return named.front();

        error = named.empty() ? t.name.quoted() + " has no effect called " + text.quoted()
                              : t.name.quoted() + " has " + juce::String ((int) named.size()) + " called " + text.quoted() + ": say which by its number";
        return std::nullopt;
    }

    const SessionTools::Parameter* SessionTools::parameterOf (const Effect& e, const juce::String& which, juce::String& error)
    {
        const auto wanted = which.trim();

        for (auto& p : e.parameters)
            if (p.id == wanted)
                return &p;

        const Parameter* found = nullptr;
        int count = 0;

        for (auto& p : e.parameters)
            if (p.name.trim().equalsIgnoreCase (wanted))
                found = &p, ++count;

        if (count == 1)
            return found;

        error = count > 1 ? e.plugin.name + " has " + juce::String (count) + " parameters called " + wanted.quoted() + ": give its id from list_effect_parameters"
                          : e.plugin.name + " has no parameter called " + wanted.quoted() + " (list_effect_parameters lists them)";
        return nullptr;
    }

    juce::String SessionTools::chainRefusal (const SessionTrack& t) const
    {
        if (t.isControl())
            return juce::String (t.isFolder ? "a folder" : "a VCA fader") + " has no effects: they go on its tracks or a bus";

        if (t.isFrozen())
            return t.name.quoted() + " is frozen: its effects can't change until it is unfrozen (freeze_track with freeze false)";

        return {};
    }

    juce::Array<juce::var> SessionTools::toolDefinitions()
    {
        const auto text = juce::String (toolsJson).replace ("TRACK", trackParam).replace ("CLIP", clipParam).replace ("SECTION", sectionParam).replace ("FXREF", effectParam);
        juce::Array<juce::var> tools;

        const auto parsed = juce::JSON::parse (text);

        if (auto* list = parsed.getArray())
            for (auto& t : *list)
                tools.add (object ({ { "type", "function" }, { "function", t } }));

        jassert (tools.size() == 37);
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
               "guessing. A track's effects: get_session lists them in order; add_track_effect, remove_track_effect and "
               "move_track_effect change the chain; list_effect_parameters reads an effect's or instrument's parameters and "
               "set_track_effect sets them or switches an effect off. Chain changes and settings are made once the plan is "
               "applied and aren't undone with it, and an effect added in the plan can be set only in a later request. Automation "
               "(set_automation) of volume, pan or an effect's parameter takes over from the setting while it has points. "
               "freeze_track and bounce_track render once the plan is applied, after everything else in it. What these tools "
               "can't do: recording, replacing an instrument, and changing the audio inside a clip (set_clip_effects puts a pitch "
               "shift and effects on a clip without changing its audio). Say so when a request needs them, rather than doing "
               "something else.";
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
        else if (tool == "comp_takes")     result = compTakes (args);
        else if (tool == "set_clip_effects") result = setClipEffects (args);
        else if (tool == "add_section")    result = addSection (args);
        else if (tool == "move_section")   result = moveSection (args);
        else if (tool == "delete_section") result = deleteSection (args);
        else if (tool == "change_section_bars") result = changeSectionBars (args);
        else if (tool == "set_arrangement") result = setArrangement (args);
        else if (tool == "remove_clip")    result = removeClip (args);
        else if (tool == "add_marker")     result = addMarker (args);
        else if (tool == "set_tempo")      result = setTempo (args);
        else if (tool == "fit_tempo_to_hits") result = fitTempoToHits (args);
        else if (tool == "set_automation") result = setAutomation (args);
        else if (tool == "add_track_effect") result = addTrackEffect (args);
        else if (tool == "remove_track_effect") result = removeTrackEffect (args);
        else if (tool == "move_track_effect") result = moveTrackEffect (args);
        else if (tool == "set_track_effect") result = setTrackEffect (args);
        else if (tool == "list_effect_parameters") result = listEffectParameters (args);
        else if (tool == "freeze_track")   result = freezeTrack (args);
        else if (tool == "bounce_track")   result = bounceTrack (args);
        else if (tool == "add_note_clip")  result = addNoteClip (args);
        else if (tool == "set_instrument") result = setInstrument (args);
        else if (tool == "list_instrument_presets") result = listInstrumentPresets (args);
        else if (tool == "store_scene")    result = storeScene (args);
        else if (tool == "recall_scene")   result = recallScene (args);
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
        return t.isFolder ? "folder" : t.isVca ? "vca" : t.isMaster ? "master" : t.isBus ? "bus" : t.isInstrument ? "instrument" : "audio";
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

            if (t.vcaId != 0)
                o->setProperty ("vca", nameOf (t.vcaId));

            o->setProperty ("volume", round1 (t.gainDb));

            if (! t.isControl())
                o->setProperty ("pan", round3 (t.pan));

            o->setProperty ("mute", t.mute);
            o->setProperty ("solo", t.solo);

            if (! t.isControl())
                o->setProperty ("output", t.outputBusId != 0 ? nameOf (t.outputBusId) : juce::String ("mix"));

            if (t.automationMode != AutomationMode::read)
                o->setProperty ("automation_mode", automationModeName (t.automationMode).toLowerCase());

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
                // As the plan leaves it: numbered from 1, as the effect tools name them.
                juce::StringArray chain;

                for (auto& slot : viewOf (t.id))
                    if (! slot.instrument)
                        chain.add (juce::String (chain.size() + 1) + ". " + slot.plugin.name + (slot.bypassed ? " (off)" : "")
                                   + (slot.slotId == 0 ? " (when the plan is applied)" : ""));

                if (! chain.isEmpty())
                    o->setProperty ("effects", chain.joinIntoString (", "));
            }

            juce::StringArray automated;

            for (const auto& [name, e] : { std::pair<const char*, const Envelope*> { "volume", &t.volumeEnvelope }, { "pan", &t.panEnvelope } })
                if (! e->isEmpty())
                    automated.add (juce::String (name) + " (" + juce::String ((int) e->points.size()) + " points, "
                                   + seconds (e->points.front().frame) + " to " + seconds (e->points.back().frame) + " s)");

            for (auto& e : t.effectEnvelopes)
            {
                juce::String what = "a parameter of a plug-in no longer on the track";

                if (const auto c = chains.find (t.id); c != chains.end())
                    for (auto& slot : c->second)
                        if (slot.slotId == e.slotId)
                        {
                            what = slot.plugin.name + "'s " + e.parameterId.quoted();

                            for (auto& p : slot.parameters)
                                if (p.id == e.parameterId)
                                    what = slot.plugin.name + "'s " + p.name.quoted();
                        }

                automated.add (what + " (" + juce::String ((int) e.envelope.points.size()) + " points)");
            }

            if (t.hasSurroundAutomation())
                automated.add ("surround pan");

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

        juce::Array<juce::var> sections, changes;
        const auto map = state.tempoMap();

        for (auto& sec : state.sections)
            sections.add (object ({ { "name", sec.name }, { "start", seconds (sec.start).getDoubleValue() }, { "end", seconds (sec.getEnd()).getDoubleValue() },
                                    { "bars", Metronome::lengthText (map, sec.start, sec.length) } }));

        for (auto& c : state.tempoChanges)
            changes.add (object ({ { "bar", (int) c.bar + 1 }, { "time", seconds (map.barFrame (c.bar)).getDoubleValue() }, { "tempo", c.tempo },
                                   { "beats_per_bar", c.beatsPerBar } }));

        juce::StringArray chain;

        for (auto id : state.arrangement)
            if (const auto* sec = state.findSection (id))
                chain.add (sec->name);

        auto result = object ({ { "name", sessionName }, { "sample_rate", state.sampleRate }, { "length", seconds (state.getLength()).getDoubleValue() },
                                { "tempo", state.tempo }, { "beats_per_bar", state.beatsPerBar }, { "tracks", tracks }, { "markers", markers } });

        if (! changes.isEmpty())
            result.getDynamicObject()->setProperty ("tempo_changes", changes);

        if (state.timecode.isSet())
            result.getDynamicObject()->setProperty ("timecode", TimeFormat::names()[(int) state.timecode.rate].fromFirstOccurrenceOf ("SMPTE ", false, false)
                                                                    + " from " + TimeFormat::countedText (state.timecode.start, state.timecode.rate));

        if (! sections.isEmpty())
            result.getDynamicObject()->setProperty ("sections", sections);

        if (! chain.isEmpty())
            result.getDynamicObject()->setProperty ("arrangement", chain.joinIntoString (", ") + (state.playArrangement ? " (playback follows it)" : ""));

        juce::StringArray scenes;

        for (auto& scene : state.scenes)
            scenes.add (scene.name);

        if (! scenes.isEmpty())
            result.getDynamicObject()->setProperty ("mixer_scenes", scenes.joinIntoString (", "));

        return result;
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
                                 { "muted", c.muted }, { "locked", c.locked }, { "takes", (int) c.takes.size() },
                                 { "effects", c.effects.isOn() ? juce::var (ClipEffects::describe (c.effects)) : juce::var() },
                                 { "playing", c.takes.size() < 2 ? juce::var() : c.comp.empty() ? juce::var ("take " + juce::String (c.activeTake + 1)) : juce::var ("a comp") } }));

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

            if (t.isControl())
                return refused (t.isFolder ? "a folder has no pan" : "a VCA fader has no pan");

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

            if (! state.tracks[(size_t) *bus].takesBusInput())
                return refused (state.tracks[(size_t) *bus].name.quoted() + " isn't a bus (the master takes the mix: use \"mix\")");

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

        if (! state.tracks[(size_t) *bus].takesBusInput())
            return refused (state.tracks[(size_t) *bus].name.quoted() + " isn't a bus (the master takes no sends)");

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

    juce::var SessionTools::setClipEffects (const juce::var& a)
    {
        const auto clipId = (juce::uint32) (int) a["clip"];
        const auto where = state.findClip (clipId);

        if (! where)
            return refused ("no audio clip with id " + a["clip"].toString());

        const auto& clip = state.tracks[(size_t) where->first].clips[(size_t) where->second];
        const auto name = clip.name;
        auto semitones = clip.effects.semitones;
        auto chain = clip.effects.chain;
        juce::StringArray what;

        if (a.hasProperty ("pitch"))
        {
            const auto& v = a["pitch"];
            semitones = v.isDouble() || v.isInt() || v.isInt64() ? (double) v : std::numeric_limits<double>::quiet_NaN();

            if (! std::isfinite (semitones) || std::abs (semitones) > SessionClip::Effects::maxSemitones)
                return refused ("pitch must be from -12 to +12 semitones");

            what.add (juce::exactlyEqual (semitones, 0.0) ? juce::String ("no pitch shift")
                                                          : "pitch " + juce::String (semitones > 0.0 ? "+" : "") + juce::String (round3 (semitones)) + " semitones");
        }

        if ((bool) a["clear"])
        {
            chain = {};
            what.add ("no effects");
        }

        if (a.hasProperty ("add"))
        {
            if (findEffect == nullptr)
                return refused ("no plug-ins are known here");

            juce::String error;
            const auto plugin = findEffect (a["add"].toString().trim(), error);

            if (! plugin)
                return refused (error);

            if (plugin->isInstrument)
                return refused (plugin->name + " is an instrument, not an effect");

            chain = ClipEffects::withSlot (chain, *plugin);
            what.add ("add " + plugin->name);
        }

        if (what.isEmpty())
            return refused ("give pitch, add or clear");

        if (! SessionEdits::setClipEffects (state, clipId, semitones, chain))
            return refused ("the clip is locked or on a frozen track, or already has those effects");

        return done ("Clip effects on " + name.quoted() + ": " + what.joinIntoString (", "));
    }

    juce::var SessionTools::compTakes (const juce::var& a)
    {
        const auto clipId = (juce::uint32) (int) a["clip"];
        const auto where = state.findClip (clipId);

        if (! where)
            return refused ("no audio clip with id " + a["clip"].toString());

        const auto& clip = state.tracks[(size_t) where->first].clips[(size_t) where->second];
        const auto name = clip.name;
        const auto take = (int) a["take"] - 1;

        if (! juce::isPositiveAndBelow (take, (int) clip.takes.size()))
            return refused (clip.takes.size() < 2 ? "the clip has no takes" : "take must be from 1 to " + juce::String ((int) clip.takes.size()));

        if (! a.hasProperty ("start") && ! a.hasProperty ("end"))
        {
            if (! SessionEdits::playTakeThroughout (state, clipId, take))
                return refused ("the clip is locked, or plays that take throughout already");

            return done ("Play take " + juce::String (take + 1) + " throughout clip " + name.quoted());
        }

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

        if (! SessionEdits::compTake (state, clipId, take, start, end))
            return refused ("the range must be inside the clip, and the clip not locked or playing that take there already");

        return done ("Comp take " + juce::String (take + 1) + " of clip " + name.quoted() + " from " + seconds (juce::jmax (start, clip.position))
                     + " s to " + seconds (juce::jmin (end, clip.getEnd())) + " s");
    }

    std::optional<juce::uint32> SessionTools::sectionOf (const juce::var& which, juce::String& error) const
    {
        const auto name = which.toString().trim();

        for (auto& sec : state.sections)
            if (sec.name.equalsIgnoreCase (name))
                return sec.id;

        juce::StringArray names;

        for (auto& sec : state.sections)
            names.add (sec.name);

        error = "no section named " + name.quoted() + (names.isEmpty() ? juce::String (" (there are none)") : " (there are " + names.joinIntoString (", ") + ")");
        return std::nullopt;
    }

    juce::var SessionTools::addSection (const juce::var& a)
    {
        const auto start = frameOf (a["start"]), end = frameOf (a["end"]);

        if (! start || ! end || *end <= *start)
            return refused ("start and end must be times in seconds, end after start");

        const auto name = a["name"].toString().trim();

        if (SessionEdits::addSection (state, *start, *end, name) == 0)
            return refused ("it would overlap another section");

        return done ("Add section " + name.quoted() + " from " + seconds (*start) + " s to " + seconds (*end) + " s");
    }

    juce::var SessionTools::moveSection (const juce::var& a)
    {
        juce::String error;
        const auto id = sectionOf (a["section"], error);
        const auto to = frameOf (a["to"]);

        if (! id)
            return refused (error);

        if (! to)
            return refused ("to must be a time in seconds");

        const auto name = state.findSection (*id)->name;
        const auto copy = (bool) a["copy"];

        if (! SessionEdits::moveSection (state, *id, *to, copy))
            return refused ("it can't go inside itself, or where it already is");

        return done ((copy ? "Copy section " : "Move section ") + name.quoted() + " to " + seconds (*to) + " s");
    }

    juce::var SessionTools::deleteSection (const juce::var& a)
    {
        juce::String error;
        const auto id = sectionOf (a["section"], error);

        if (! id)
            return refused (error);

        const auto name = state.findSection (*id)->name;
        SessionEdits::deleteSection (state, *id);
        return done ("Delete section " + name.quoted() + " and its time");
    }

    juce::var SessionTools::changeSectionBars (const juce::var& a)
    {
        juce::String error;
        const auto id = sectionOf (a["section"], error);
        const auto bars = (int) a["bars"];
        const auto atStart = a["at"].toString() == "start";

        if (! id)
            return refused (error);

        if (bars == 0 || std::abs (bars) > 999)
            return refused ("bars must be a number of bars to add or take away, not 0");

        const auto sec = *state.findSection (*id);
        const auto map = state.tempoMap();

        if (bars > 0)
        {
            // Inserted at its start the section moves later, at its end it stays: either way it then
            // spans its old start to its old end plus the new bars.
            const auto added = SessionEdits::insertBars (state, atStart ? sec.start : sec.getEnd(), bars);

            if (added == 0)
                return refused ("no bars could go in there");

            SessionEdits::resizeSection (state, *id, sec.start, sec.getEnd() + added);
            return done ("Add " + juce::String (bars) + " bars at the " + (atStart ? "start" : "end") + " of section " + sec.name.quoted());
        }

        // Fewer: whole bars from the start or back from the end, inside the section.
        const auto firstBar = map.barAt (sec.start), endBar = map.barAt (sec.getEnd() - 1) + 1;
        const auto from = atStart ? sec.start : map.barFrame (endBar + bars);
        const auto to = atStart ? map.barFrame (firstBar - bars) : sec.getEnd();

        if (from < sec.start || to > sec.getEnd() || to - from >= sec.length)
            return refused ("the section has fewer bars than that");

        SessionEdits::deleteTime (state, from, to);
        return done ("Take " + juce::String (-bars) + " bars from the " + (atStart ? "start" : "end") + " of section " + sec.name.quoted());
    }

    juce::var SessionTools::setArrangement (const juce::var& a)
    {
        std::vector<juce::uint32> chain;

        if (const auto* names = a["sections"].getArray())
        {
            for (auto& name : *names)
            {
                juce::String error;
                const auto id = sectionOf (name, error);

                if (! id)
                    return refused (error);

                chain.push_back (*id);
            }
        }

        const auto chainChanged = SessionEdits::setArrangement (state, chain);
        const auto playChanged = SessionEdits::setPlayArrangement (state, (bool) a["play"] && ! chain.empty());

        if (! chainChanged && ! playChanged)
            return refused ("the arrangement is so already");

        juce::StringArray names;

        for (auto id : chain)
            names.add (state.findSection (id)->name);

        return done (chain.empty() ? juce::String ("Clear the arrangement chain")
                                   : "Set the arrangement chain to " + names.joinIntoString (", ") + ((bool) a["play"] ? ", and play it" : ""));
    }

    std::optional<juce::uint32> SessionTools::sceneNamed (const juce::String& name) const
    {
        for (auto& scene : state.scenes)
            if (scene.name.trim().equalsIgnoreCase (name.trim()))
                return scene.id;

        return std::nullopt;
    }

    juce::var SessionTools::storeScene (const juce::var& a)
    {
        const auto name = a["name"].toString().trim();

        if (name.isEmpty())
            return refused ("the scene needs a name");

        if (const auto id = sceneNamed (name))
        {
            if (! SessionEdits::updateScene (state, *id))
                return refused ("the scene " + name.quoted() + " holds this mix already");

            return done ("Store the mix into the scene " + name.quoted());
        }

        SessionEdits::storeScene (state, name);
        return done ("Store the mix as the scene " + name.quoted());
    }

    juce::var SessionTools::recallScene (const juce::var& a)
    {
        const auto name = a["name"].toString().trim();
        const auto id = sceneNamed (name);

        if (! id)
        {
            juce::StringArray names;

            for (auto& scene : state.scenes)
                names.add (scene.name);

            return refused ("no mixer scene named " + name.quoted() + (names.isEmpty() ? juce::String ("; the session has none")
                                                                                        : "; there are " + names.joinIntoString (", ")));
        }

        if (! SessionEdits::recallScene (state, *id))
            return refused ("the mix is the scene " + name.quoted() + "'s already");

        return done ("Recall the mixer scene " + name.quoted());
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

    juce::var SessionTools::fitTempoToHits (const juce::var& a)
    {
        std::vector<juce::int64> hits;
        juce::StringArray names;

        if (const auto* wanted = a["markers"].getArray())
        {
            for (auto& name : *wanted)
            {
                const auto m = std::find_if (state.markers.begin(), state.markers.end(),
                                             [n = name.toString().trim()] (const Marker& x) { return ! x.isRegion() && x.name.trim().equalsIgnoreCase (n); });

                if (m == state.markers.end())
                    return refused ("no point marker named " + name.toString().quoted() + "; get_session lists the markers");

                hits.push_back (m->start);
                names.add (m->name);
            }
        }
        else
        {
            for (auto& m : state.markers)
                if (! m.isRegion())
                {
                    hits.push_back (m.start);
                    names.add (m.name);
                }
        }

        if (hits.empty())
            return refused ("the session has no point markers to fit the tempo to");

        std::vector<size_t> order (hits.size());
        std::iota (order.begin(), order.end(), (size_t) 0);
        std::sort (order.begin(), order.end(), [&hits] (size_t x, size_t y) { return hits[x] < hits[y]; });
        std::vector<juce::int64> sorted;

        for (auto i : order)
            sorted.push_back (hits[i]);

        const auto landOnText = a["land_on"].toString();
        const auto landOn = landOnText == "bars" ? Picture::LandOn::bars : landOnText == "bar_lines" ? Picture::LandOn::barLines : Picture::LandOn::beats;
        const auto low = a.hasProperty ("min_bpm") ? (double) a["min_bpm"] : juce::jmax (20.0, std::floor (state.tempo * 0.8));
        const auto high = a.hasProperty ("max_bpm") ? (double) a["max_bpm"] : juce::jmin (400.0, std::ceil (state.tempo * 1.25));
        const auto frame = (juce::int64) std::llround (state.sampleRate / (state.timecode.isSet() ? TimeFormat::framesPerSecond (state.timecode.rate) : 25.0));
        const auto fit = Picture::fitTempo (sorted, state.sampleRate, state.beatsPerBar, landOn, low, high, frame);

        if (! std::isfinite (low) || ! std::isfinite (high) || ! fit.isValid())
            return refused ("min_bpm and max_bpm must be 20 to 400, the lower first");

        if (! SessionEdits::applyTempoFit (state, fit))
            return refused ("the tempo is so already");

        juce::Array<juce::var> landed;

        for (size_t i = 0; i < order.size(); ++i)
            landed.add (object ({ { "marker", names[(int) order[i]] }, { "off_ms", round1 (1000.0 * (double) fit.misses[i] / state.sampleRate) } }));

        auto result = done ("Fit the tempo to " + juce::String ((int) hits.size()) + (hits.size() == 1 ? " hit" : " hits") + ": "
                            + juce::String (fit.tempo, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".") + " bpm"
                            + (fit.changes.empty() ? juce::String() : " from the start, with " + juce::String ((int) fit.changes.size()) + " tempo changes"));
        result.getDynamicObject()->setProperty ("tempo", round3 (fit.tempo));
        result.getDynamicObject()->setProperty ("within_a_frame", fit.within);
        result.getDynamicObject()->setProperty ("hits", landed);
        return result;
    }

    juce::var SessionTools::setAutomation (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];
        const auto which = a["parameter"].toString().trim();
        const auto onEffect = ! a["effect"].isVoid() && a["effect"].toString().isNotEmpty();
        AutomationTarget target;
        juce::String what;

        if (onEffect)
        {
            if (const auto refusal = chainRefusal (t); refusal.isNotEmpty())
                return refused (refusal);

            const auto slot = effectOf (t.id, a["effect"], error);

            if (! slot)
                return refused (error);

            const auto s = viewOf (t.id)[*slot];

            if (s.was == nullptr)
                return refused (s.plugin.name + " is added in this plan, so its parameters can be automated only once it is there, in a later request");

            const auto* parameter = parameterOf (*s.was, which, error);

            if (parameter == nullptr)
                return refused (error);

            target = AutomationTarget::effect (s.slotId, parameter->id);
            what = s.plugin.name + "'s " + parameter->name.quoted();
        }
        else
        {
            if (which != "volume" && which != "pan")
                return refused ("parameter must be volume or pan, or give effect with one of its parameters");

            if (t.isFolder || (t.isVca && which == "pan"))
                return refused (juce::String (t.isFolder ? "a folder" : "a VCA fader") + " has no " + which + " to automate");

            target = which == "volume" ? AutomationParameter::volume : AutomationParameter::pan;
            what = which;
        }

        const auto range = automationRange (target.parameter);
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
                return refused (onEffect ? "an effect's parameter points must be 0 to 1"
                                         : which == "volume" ? "volume points must be -60 to +12 dB" : "pan points must be -1 to +1");

            e.points.push_back ({ *frame, (float) v });
        }

        std::stable_sort (e.points.begin(), e.points.end(), [] (auto& x, auto& y) { return x.frame < y.frame; });

        if (t.envelope (target) == e)
            return refused ("the " + what + " automation is so already");

        if (! SessionEdits::setEnvelope (state, t.id, target, std::move (e)))
            return refused ("the " + what + " automation couldn't be set");

        const auto count = (int) points->size();
        return done (count == 0 ? "Remove the " + what + " automation on " + t.name.quoted()
                                : "Automate the " + what + " on " + t.name.quoted() + " with " + juce::String (count) + " point" + (count == 1 ? "" : "s"));
    }

    juce::var SessionTools::addTrackEffect (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (const auto refusal = chainRefusal (t); refusal.isNotEmpty())
            return refused (refusal);

        if (findEffect == nullptr)
            return refused ("no plug-ins are known here");

        const auto plugin = findEffect (a["plugin"].toString().trim(), error);

        if (! plugin)
            return refused (error);

        if (plugin->isInstrument)
            return refused (plugin->name + " is an instrument: an instrument track's instrument is chosen with set_instrument");

        auto& chain = chainOf (t.id);
        const auto first = (int) (! chain.empty() && chain.front().instrument);
        const auto count = (int) chain.size() - first;
        auto at = (int) chain.size();

        if (! a["position"].isVoid())
        {
            const auto position = (int) a["position"];

            if (! juce::isPositiveAndNotGreaterThan (position, count + 1))
                return refused ("position must be 1 to " + juce::String (count + 1));

            at = first + position - 1;
        }

        PlannedSlot slot;
        slot.plugin = *plugin;
        chain.insert (chain.begin() + at, slot);
        return done ("Add " + plugin->name + " to the effects on " + t.name.quoted()
                     + (at == (int) chain.size() - 1 ? juce::String() : ", as effect " + juce::String (at - first + 1)) + " (when the plan is applied)");
    }

    juce::var SessionTools::removeTrackEffect (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (const auto refusal = chainRefusal (t); refusal.isNotEmpty())
            return refused (refusal);

        const auto slot = effectOf (t.id, a["effect"], error);

        if (! slot)
            return refused (error);

        auto& chain = chainOf (t.id);

        if (chain[*slot].instrument)
            return refused ("an instrument track's instrument stays; only its effects can be taken off");

        const auto removed = chain[*slot];
        chain.erase (chain.begin() + (std::ptrdiff_t) *slot);

        // Its automation goes with it, rather than staying behind unused.
        if (removed.slotId != 0)
        {
            std::vector<juce::String> automated;

            for (auto& e : t.effectEnvelopes)
                if (e.slotId == removed.slotId)
                    automated.push_back (e.parameterId);

            for (auto& id : automated)
                SessionEdits::setEnvelope (state, t.id, AutomationTarget::effect (removed.slotId, id), {});
        }

        return done ("Take " + removed.plugin.name + " off " + t.name.quoted() + (removed.slotId == 0 ? juce::String() : " (when the plan is applied)"));
    }

    juce::var SessionTools::moveTrackEffect (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (const auto refusal = chainRefusal (t); refusal.isNotEmpty())
            return refused (refusal);

        const auto slot = effectOf (t.id, a["effect"], error);

        if (! slot)
            return refused (error);

        auto& chain = chainOf (t.id);

        if (chain[*slot].instrument)
            return refused ("an instrument track's instrument stays first in its chain");

        const auto first = (int) chain.front().instrument;
        const auto count = (int) chain.size() - first;
        const auto to = a["to"].isVoid() ? 0 : (int) a["to"];

        if (! juce::isPositiveAndNotGreaterThan (to, count))
            return refused ("to must be 1 to " + juce::String (count));

        const auto at = (size_t) (first + to - 1);

        if (at == *slot)
            return refused (chain[*slot].plugin.name + " is effect " + juce::String (to) + " already");

        auto moving = chain[*slot];
        chain.erase (chain.begin() + (std::ptrdiff_t) *slot);
        chain.insert (chain.begin() + (std::ptrdiff_t) at, moving);
        return done ("Move " + moving.plugin.name + " to effect " + juce::String (to) + " on " + t.name.quoted() + " (when the plan is applied)");
    }

    juce::var SessionTools::listEffectParameters (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (t.isControl())
            return refused (chainRefusal (t));

        const auto slot = effectOf (t.id, a["effect"], error);

        if (! slot)
            return refused (error);

        const auto s = viewOf (t.id)[*slot];

        if (s.was == nullptr)
            return refused (s.plugin.name + " is added in this plan: its parameters can be read once it is there, in a later request");

        const auto find = a["find"].toString().trim();
        constexpr int most = 150;
        juce::Array<juce::var> list;
        int matching = 0;

        for (auto& p : s.was->parameters)
        {
            if (find.isNotEmpty() && ! p.name.containsIgnoreCase (find) && ! p.id.containsIgnoreCase (find))
                continue;

            if (++matching > most)
                continue;

            auto* o = new juce::DynamicObject();
            o->setProperty ("id", p.id);
            o->setProperty ("name", p.name);
            o->setProperty ("value", round3 (p.value));
            o->setProperty ("reads", p.text);
            o->setProperty ("range", p.lowest + " to " + p.highest);

            if (! p.choices.isEmpty())
                o->setProperty ("choices", p.choices.joinIntoString (" | "));

            // What the plan sets it to, if anything.
            for (auto& setting : s.settings)
                if (setting.parameterId == p.id)
                    o->setProperty ("planned", setting.value ? juce::var (round3 (*setting.value)) : juce::var (setting.text));

            list.add (juce::var (o));
        }

        auto result = object ({ { "plugin", s.plugin.name }, { "bypassed", s.bypassed }, { "parameters", list } });

        if (matching > most)
            result.getDynamicObject()->setProperty ("note", "the first " + juce::String (most) + " of " + juce::String (matching) + "; narrow with find");

        return result;
    }

    juce::var SessionTools::setTrackEffect (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (const auto refusal = chainRefusal (t); refusal.isNotEmpty())
            return refused (refusal);

        const auto slot = effectOf (t.id, a["effect"], error);

        if (! slot)
            return refused (error);

        // Checked whole before anything changes, so a refused call leaves the plan as it was.
        auto changed = viewOf (t.id)[*slot];
        juce::StringArray what;

        if (! a["bypass"].isVoid())
        {
            if (changed.instrument)
                return refused ("an instrument can't be switched off; mute the track instead");

            const auto bypass = (bool) a["bypass"];

            if (bypass != changed.bypassed)
                what.add (bypass ? "off" : "on");

            changed.bypassed = bypass;
        }

        if (const auto* list = a["parameters"].getArray(); list != nullptr && ! list->isEmpty())
        {
            if (changed.was == nullptr)
                return refused (changed.plugin.name + " is added in this plan, so its settings can be changed only once it is there, in a later request");

            for (auto& item : *list)
            {
                const auto* p = parameterOf (*changed.was, item["parameter"].toString(), error);

                if (p == nullptr)
                    return refused (error);

                Setting setting;
                setting.parameterId = p->id;
                const auto& value = item["value"];
                const auto text = item["text"].toString().trim();

                if (value.isDouble() || value.isInt() || value.isInt64())
                {
                    const auto v = (double) value;

                    if (! std::isfinite (v) || v < 0.0 || v > 1.0)
                        return refused (p->name.quoted() + ": a value is 0 to 1 (or give text, as it reads)");

                    setting.value = (float) v;
                    what.add (p->name.quoted() + " to " + juce::String (round3 (v)));
                }
                else if (text.isNotEmpty())
                {
                    // A stepped parameter's choice is its value; other text the plug-in reads when it is set.
                    if (! p->choices.isEmpty())
                    {
                        const auto choice = p->choices.indexOf (text, true);

                        if (choice < 0)
                            return refused (p->name.quoted() + " is one of " + p->choices.joinIntoString (", "));

                        setting.value = p->choices.size() > 1 ? (float) choice / (float) (p->choices.size() - 1) : 0.0f;
                    }
                    else
                    {
                        setting.text = text;
                    }

                    what.add (p->name.quoted() + " to " + text.quoted());
                }
                else
                {
                    return refused (p->name.quoted() + " needs a value (0 to 1) or text");
                }

                auto& settings = changed.settings;
                settings.erase (std::remove_if (settings.begin(), settings.end(), [&setting] (const Setting& x) { return x.parameterId == setting.parameterId; }),
                                settings.end());
                settings.push_back (setting);
            }
        }

        if (what.isEmpty())
            return refused ("give bypass or parameters to change");

        chainOf (t.id)[*slot] = changed;
        return done ("Set " + changed.plugin.name + " on " + t.name.quoted() + ": " + what.joinIntoString (", ") + " (when the plan is applied)");
    }

    juce::var SessionTools::freezeTrack (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (t.isBus || t.isControl())
            return refused ("only a track can be frozen, not a bus, folder or VCA fader");

        const auto queued = std::find_if (renders.begin(), renders.end(), [&t] (const Render& r) { return r.trackId == t.id && r.freeze; });

        if (! a["freeze"].isVoid() && ! (bool) a["freeze"])
        {
            if (queued != renders.end())
            {
                renders.erase (queued);
                steps.removeString ("Freeze " + t.name.quoted() + " (rendered once the plan is applied)");
                return object ({ { "ok", true }, { "note", "the freeze planned for " + t.name.quoted() + " is taken out of the plan" } });
            }

            if (! t.isFrozen())
                return refused (t.name.quoted() + " isn't frozen");

            if (! SessionEdits::unfreezeTrack (state, t.id))
                return refused (t.name.quoted() + " couldn't be unfrozen");

            return done ("Unfreeze " + t.name.quoted());
        }

        if (t.isFrozen() || queued != renders.end())
            return refused (t.name.quoted() + " is frozen already" + juce::String (queued != renders.end() ? " in this plan" : ""));

        if (t.clipsEnd() == 0)
            return refused (t.name.quoted() + " has no clips to freeze");

        renders.push_back ({ t.id, true, {} });
        return done ("Freeze " + t.name.quoted() + " (rendered once the plan is applied)");
    }

    juce::var SessionTools::bounceTrack (const juce::var& a)
    {
        juce::String error;
        const auto index = trackOf (a["track"], error);

        if (! index)
            return refused (error);

        const auto& t = state.tracks[(size_t) *index];

        if (t.isBus || t.isControl())
            return refused ("only a track can be bounced, not a bus, folder or VCA fader");

        juce::Range<juce::int64> range;

        if (! a["start"].isVoid() || ! a["end"].isVoid())
        {
            const auto start = a["start"].isVoid() ? std::optional<juce::int64> (0) : frameOf (a["start"]);
            const auto end = a["end"].isVoid() ? std::optional<juce::int64> (t.clipsEnd()) : frameOf (a["end"]);

            if (! start || ! end || *end <= *start)
                return refused ("start and end are seconds, start before end");

            range = { *start, *end };
        }
        else if (t.clipsEnd() == 0)
        {
            return refused (t.name.quoted() + " has no clips to bounce");
        }

        renders.push_back ({ t.id, false, range });
        return done ("Bounce " + t.name.quoted() + (range.isEmpty() ? juce::String() : " from " + seconds (range.getStart()) + " to " + seconds (range.getEnd()) + " s")
                     + " onto a new track (rendered once the plan is applied)");
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

        const auto step = alreadyLoaded ? "Load the preset " + presetName.quoted() + " in " + plugin->name + " on " + t.name.quoted()
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
