#include "AiSequence.h"
#include "AiWorkspace.h"
#include "NoteTools.h"
#include "PluginCatalogue.h"
#include "SequencerView.h"

namespace
{
    using SequenceFile::ticksPerBeat;

    juce::var object (std::initializer_list<std::pair<const char*, juce::var>> properties)
    {
        auto* o = new juce::DynamicObject();

        for (auto& [name, value] : properties)
            o->setProperty (name, value);

        return juce::var (o);
    }

    juce::var refused (const juce::String& error)    { return object ({ { "ok", false }, { "error", error } }); }

    bool isNumber (const juce::var& v)                { return v.isInt() || v.isInt64() || v.isDouble(); }

    /** A whole number given as a number (5 or 5.0), or nothing. */
    std::optional<juce::int64> wholeNumber (const juce::var& v)
    {
        if (! isNumber (v) || ! std::isfinite ((double) v))
            return std::nullopt;

        const auto d = (double) v;

        if (! juce::exactlyEqual (std::floor (d), d) || std::abs (d) > 1.0e12)
            return std::nullopt;

        return (juce::int64) d;
    }

    constexpr int maxNotes = 4096, maxPoints = 2048, maxSuggestions = 12, maxParameters = 64;

    const char* notesSchema = R"JSON({ "type": "array", "items": { "type": "object", "properties": {
        "pitch": { "type": "integer", "description": "MIDI note, 0 to 127: 60 is middle C, C4 (so C2 is 36)" },
        "start": { "type": "integer", "description": "Ticks from the sequence's start (960 to a beat)" },
        "length": { "type": "integer", "description": "Ticks, at least 1" },
        "velocity": { "type": "integer", "description": "1 to 127; 100 when left out" },
        "probability": { "type": "number", "description": "The chance it plays on each pass, above 0 to 1; 1 when left out" } },
        "required": ["pitch", "start", "length"] } })JSON";

    const char* lanesSchema = R"JSON({ "type": "array", "items": { "type": "object", "properties": {
        "controller": { "type": "integer", "description": "A MIDI CC, 0 to 127 (1 mod wheel, 7 volume, 11 expression, 64 sustain, 71 resonance, 74 cutoff), or 128 for pitch bend" },
        "points": { "type": "array", "items": { "type": "object", "properties": { "tick": { "type": "integer" },
                    "value": { "type": "number", "description": "0 to 1: a CC's 0 to 127; pitch bend's centre is 0.5" } }, "required": ["tick", "value"] } } },
        "required": ["controller", "points"] } })JSON";

    const char* toolsJson = R"JSON([
  { "name": "get_sequence", "description": "The sequence open in the Sequencer: its name, role (what the part is for: Drums, Bass, Pad...), key, beats in a bar, length and loop in ticks (960 to a beat), the tempo, the voice's instrument and what it is, its effects, every note (pitch, its name, start, length, velocity 1 to 127, probability) and controller lane, and the notes outside the key. Call this first.",
    "parameters": { "type": "object", "properties": {} } },
  { "name": "write_sequence", "description": "Write the sequence: the notes given replace all its notes, exactly as given, and the lanes, when given, replace all its lanes. Optionally set its length in ticks (whole beats), its loop length (0 for none) and its key. It goes into the Sequencer as one undo step when the user applies the plan.",
    "parameters": { "type": "object", "properties": { "notes": NOTES, "lanes": LANES,
                    "length": { "type": "integer", "description": "Ticks, a whole number of beats; leave out to keep the length" },
                    "loop_length": { "type": "integer", "description": "Ticks the notes repeat after, 0 for none; leave out to keep it" },
                    "key": { "type": "string", "description": "Such as \"A minor\" or \"F# dorian\"; leave out to keep it" } },
                    "required": ["notes"] } },
  { "name": "suggest_for_sequence", "description": "Give the user a short review of the sequence and suggestions, each one shown with an Apply button (and Apply All for all of them as one undo step). Nothing changes until the user applies one. Each suggestion stands alone: two may not change the same note. A suggestion is one kind: notes (notes to remove, change or add; a note is found by its pitch and start), lanes (controller lanes to write, each replacing that controller's lane), effect (an effect plug-in from list_plugins added at the end of the voice with its default settings), or automation (an instrument parameter's values over time: advice only, since a sequence can't keep automation; prefer a lane when the instrument answers a controller).",
    "parameters": { "type": "object", "properties": {
        "review": { "type": "string", "description": "A few sentences against the part's role: rhythm and groove, the spread of velocities, voicing and range, notes outside the key (name them), crowding, repetition" },
        "suggestions": { "type": "array", "items": { "type": "object", "properties": {
            "title": { "type": "string", "description": "A few words, such as \"Softer ghost notes\"" },
            "why": { "type": "string" },
            "kind": { "type": "string", "enum": ["notes", "lanes", "effect", "automation"] },
            "remove": { "type": "array", "items": { "type": "object", "properties": { "pitch": { "type": "integer" }, "start": { "type": "integer" } }, "required": ["pitch", "start"] } },
            "change": { "type": "array", "items": { "type": "object", "properties": { "pitch": { "type": "integer" }, "start": { "type": "integer" },
                        "to": { "type": "object", "description": "Only what changes: pitch, start, length, velocity (1 to 127), probability",
                                "properties": { "pitch": { "type": "integer" }, "start": { "type": "integer" }, "length": { "type": "integer" },
                                                "velocity": { "type": "integer" }, "probability": { "type": "number" } } } },
                        "required": ["pitch", "start", "to"] } },
            "add": NOTES,
            "lanes": LANES,
            "effect": { "type": "string", "description": "For kind effect: the plug-in's id or name from list_plugins" },
            "parameter": { "type": "string", "description": "For kind automation: the instrument parameter's name from get_sequence" },
            "points": { "type": "array", "description": "For kind automation: values 0 to 1 at ticks", "items": { "type": "object",
                        "properties": { "tick": { "type": "integer" }, "value": { "type": "number" } }, "required": ["tick", "value"] } } },
            "required": ["title", "kind"] } } },
        "required": ["review", "suggestions"] } }
])JSON";

    juce::String kindName (Ai::SequenceTools::Suggestion::Kind k)
    {
        using Kind = Ai::SequenceTools::Suggestion::Kind;
        return k == Kind::notes ? "notes" : k == Kind::lanes ? "lanes" : k == Kind::effect ? "effect" : "automation";
    }

    /** The first note at this pitch starting on this tick. */
    std::vector<SequenceFile::Note>::iterator findNote (std::vector<SequenceFile::Note>& notes, int pitch, juce::int64 start)
    {
        return std::find_if (notes.begin(), notes.end(), [&] (const SequenceFile::Note& n) { return n.pitch == pitch && n.start == start; });
    }

    /** The notes and lanes, and what places them, the same: the voice and the names aside. */
    bool sameMusic (const SequenceFile::Sequence& a, const SequenceFile::Sequence& b)
    {
        return a.notes == b.notes && a.lanes == b.lanes && a.length == b.length && a.loopLength == b.loopLength && a.offset == b.offset
            && a.key == b.key && a.beatsPerBar == b.beatsPerBar && a.name == b.name;
    }

    /** A number of beats as written: "2", "2.5", "1.333". */
    juce::String beatsText (double beats)
    {
        auto text = juce::String (beats, 3);

        while (text.containsChar ('.') && (text.endsWithChar ('0') || text.endsWithChar ('.')))
            text = text.dropLastCharacters (1);

        return text;
    }

    juce::String ticksText (juce::int64 ticks)
    {
        const auto beats = (double) ticks / ticksPerBeat;
        return beatsText (beats) + (juce::exactlyEqual (beats, 1.0) ? " beat" : " beats");
    }
}

namespace Ai
{
    //==============================================================================
    juce::String SequenceTools::Suggestion::describe (int beatsPerBar) const
    {
        juce::String what;

        switch (kind)
        {
            case Kind::notes:
            {
                juce::StringArray parts;

                if (! change.empty())  parts.add ("changes " + juce::String ((int) change.size()) + (change.size() == 1 ? " note" : " notes"));
                if (! remove.empty())  parts.add ("takes out " + juce::String ((int) remove.size()) + (remove.size() == 1 ? " note" : " notes"));
                if (! add.empty())     parts.add ("adds " + juce::String ((int) add.size()) + (add.size() == 1 ? " note" : " notes"));

                what = parts.joinIntoString (", ");
                break;
            }

            case Kind::lanes:
            {
                juce::StringArray names;

                for (auto& l : lanes)
                    names.add (ControllerLane::nameOf (l.controller));

                what = "writes " + names.joinIntoString (", ");
                break;
            }

            case Kind::effect:
                what = "adds " + effect.name + " to the voice";
                break;

            case Kind::automation:
            {
                juce::StringArray at;

                for (auto& p : points)
                    at.add (juce::String (p.value, 2) + " at " + placeOf (p.tick, beatsPerBar));

                what = "advice only: draw " + parameter + " on the instrument's track in a session (" + at.joinIntoString ("; ") + ")";
                break;
            }
        }

        return title + (what.isNotEmpty() ? " (" + what + ")" : juce::String()) + (why.isNotEmpty() ? ". " + why : juce::String());
    }

    //==============================================================================
    SequenceTools::SequenceTools (Context c, juce::StringArray& planSteps, FindEffect find)
        : context (std::move (c)), sequence (context.sequence), steps (planSteps), findEffect (std::move (find))
    {
    }

    juce::Array<juce::var> SequenceTools::toolDefinitions()
    {
        const auto text = juce::String (toolsJson).replace ("NOTES", notesSchema).replace ("LANES", lanesSchema);
        const auto parsed = juce::JSON::parse (text);
        juce::Array<juce::var> tools;

        if (auto* list = parsed.getArray())
            for (auto& t : *list)
                tools.add (object ({ { "type", "function" }, { "function", t } }));

        jassert (tools.size() == 3);
        return tools;
    }

    juce::String SequenceTools::instructions()
    {
        return "You are working on a sequence: one part (a drum pattern, a bass line, a pad...) open in the Sequencer, written in ticks, "
               "960 to a beat, so a 4/4 bar is 3840 ticks and a sixteenth 240. Pitches are MIDI notes, 60 being middle C (C4). Velocity "
               "is 1 to 127. Write for the part's role and its instrument, in its key unless asked otherwise, and inside its length. "
               "Drum instruments mostly follow General MIDI: kick 36, snare 38, clap 39, closed hi-hat 42, open hi-hat 46, crash 49, ride 51. "
               "Asked to write, call get_sequence, then write_sequence once with the whole part; it goes into the plan, and into the "
               "Sequencer as one undo step when the user applies it. Asked to look at, review or improve it, call get_sequence, then "
               "suggest_for_sequence once with a short review and up to " + juce::String (maxSuggestions) + " suggestions; the user applies "
               "them one at a time or all together. In a review, name every note outside the key (get_sequence lists them under outside_key) "
               "and say whether it works as a passing or chromatic note. Effects for the voice come from list_plugins, with their default "
               "settings. A sequence can't keep automation of the instrument's parameters, so give those as advice (kind automation), or "
               "as a controller lane when the instrument answers one. Then stop and say in a sentence or two what you did.";
    }

    std::optional<juce::var> SequenceTools::run (const juce::String& tool, const juce::var& args)
    {
        if (tool == "get_sequence")            return getSequenceTool();
        if (tool == "write_sequence")          return writeSequence (args);
        if (tool == "suggest_for_sequence")    return suggest (args);

        return std::nullopt;
    }

    //==============================================================================
    juce::String SequenceTools::noteName (int pitch)
    {
        return juce::MidiMessage::getMidiNoteName (pitch, true, true, 4);
    }

    juce::String SequenceTools::placeOf (juce::int64 tick, int beatsPerBar)
    {
        const auto bar = (juce::int64) ticksPerBeat * juce::jmax (1, beatsPerBar);
        return "bar " + juce::String (tick / bar + 1) + ", beat " + beatsText ((double) (tick % bar) / ticksPerBeat + 1.0);
    }

    juce::StringArray SequenceTools::notesOutsideKey (const SequenceFile::Sequence& q)
    {
        const auto scale = NoteTools::parseKey (q.key);
        juce::StringArray outside;

        if (! scale)
            return outside;

        for (auto& n : q.notes)
            if (! scale->contains (n.pitch))
                outside.add (noteName (n.pitch) + " at " + placeOf (n.start, q.beatsPerBar));

        return outside;
    }

    juce::var SequenceTools::describe (const Context& c, const SequenceFile::Sequence& q)
    {
        juce::Array<juce::var> notes, lanes;

        for (auto& n : q.notes)
            notes.add (object ({ { "pitch", n.pitch }, { "name", noteName (n.pitch) }, { "start", n.start }, { "length", n.length },
                                 { "velocity", juce::jlimit (1, 127, juce::roundToInt (n.velocity * 127.0f)) },
                                 { "probability", std::round ((double) n.probability * 1000.0) / 1000.0 } }));

        for (auto& l : q.lanes)
        {
            juce::Array<juce::var> points;

            for (auto& p : l.points)
                points.add (object ({ { "tick", p.tick }, { "value", std::round ((double) p.value * 1000.0) / 1000.0 } }));

            lanes.add (object ({ { "controller", l.controller }, { "name", ControllerLane::nameOf (l.controller) }, { "points", points } }));
        }

        juce::StringArray parameters;

        for (int i = 0; i < juce::jmin (maxParameters, c.instrumentParameters.size()); ++i)
            parameters.add (c.instrumentParameters[i]);

        auto result = object ({ { "ok", true },
                                { "name", q.name },
                                { "role", q.role },
                                { "sub_role", q.subRole },
                                { "key", q.key.isNotEmpty() ? juce::var (q.key) : juce::var ("none set") },
                                { "beats_per_bar", q.beatsPerBar },
                                { "ticks_per_beat", ticksPerBeat },
                                { "length", q.length },
                                { "bars", std::round (q.bars() * 1000.0) / 1000.0 },
                                { "loop_length", q.loopLength },
                                { "tempo", c.tempo },
                                { "instrument", c.instrument.isNotEmpty() ? juce::var (c.instrument) : juce::var ("none: it plays nothing until one is chosen") },
                                { "instrument_parameters", parameters },
                                { "effects", c.effects },
                                { "notes", notes },
                                { "lanes", lanes },
                                { "outside_key", notesOutsideKey (q) } });
        auto* o = result.getDynamicObject();

        if (c.instrumentAbout.isNotEmpty())
            o->setProperty ("instrument_about", c.instrumentAbout);

        if (c.sessionTempo > 0.0)
            o->setProperty ("session_tempo", c.sessionTempo);

        if (q.note.isNotEmpty())
            o->setProperty ("note", q.note);

        return result;
    }

    juce::var SequenceTools::getSequenceTool()
    {
        return describe (context, sequence);
    }

    //==============================================================================
    std::optional<SequenceFile::Note> SequenceTools::readNote (const juce::var& n, juce::int64 length, const juce::String& which, juce::String& error) const
    {
        const auto pitch = wholeNumber (n["pitch"]), start = wholeNumber (n["start"]), noteLength = wholeNumber (n["length"]);
        const auto velocity = n["velocity"].isVoid() ? 100.0 : (isNumber (n["velocity"]) ? (double) n["velocity"] : -1.0);
        const auto probability = n["probability"].isVoid() ? 1.0 : (isNumber (n["probability"]) ? (double) n["probability"] : -1.0);
        const auto room = sequence.loopLength > 0 ? juce::jmin (sequence.loopLength, length) : length;

        if (! pitch || *pitch < 0 || *pitch > 127)
            error = which + "pitch is a whole number from 0 to 127";
        else if (! start || ! noteLength || *start < 0 || *noteLength < 1)
            error = which + "start and length are whole numbers of ticks, start from 0 and length at least 1";
        else if (*start >= room)
            error = which + "starts at tick " + juce::String (*start) + ", past the " + (sequence.loopLength > 0 ? "loop's" : "sequence's")
                  + " length of " + juce::String (room) + " ticks, so it would never play";
        else if (*start + *noteLength > length)
            error = which + "ends at tick " + juce::String (*start + *noteLength) + ", past the sequence's end at " + juce::String (length);
        else if (! std::isfinite (velocity) || velocity < 1.0 || velocity > 127.0)
            error = which + "velocity is 1 to 127";
        else if (! std::isfinite (probability) || probability <= 0.0 || probability > 1.0)
            error = which + "probability is above 0, up to 1";

        if (error.isNotEmpty())
            return std::nullopt;

        return SequenceFile::Note { (int) *pitch, *start, *noteLength, (float) (velocity / 127.0), (float) probability };
    }

    std::optional<SequenceFile::Lane> SequenceTools::readLane (const juce::var& l, juce::int64 length, const juce::String& which, juce::String& error) const
    {
        const auto controller = wholeNumber (l["controller"]);
        const auto* points = l["points"].getArray();

        if (! controller || *controller < 0 || *controller > ControllerLane::pitchBend)
            error = which + "controller is a CC from 0 to 127, or 128 for pitch bend";
        else if (points == nullptr || points->isEmpty() || points->size() > maxPoints)
            error = which + "points is a list of 1 to " + juce::String (maxPoints) + " points, each with tick and value";

        if (error.isNotEmpty())
            return std::nullopt;

        SequenceFile::Lane lane { (int) *controller, {} };

        for (int i = 0; i < points->size(); ++i)
        {
            const auto& p = (*points)[i];
            const auto tick = wholeNumber (p["tick"]);
            const auto value = isNumber (p["value"]) ? (double) p["value"] : -1.0;
            const auto at = which + "point " + juce::String (i + 1) + ": ";

            if (! tick || *tick < 0 || *tick > length)
                error = at + "tick is a whole number from 0 to the sequence's length, " + juce::String (length);
            else if (! std::isfinite (value) || value < 0.0 || value > 1.0)
                error = at + "value is 0 to 1";
            else if (! lane.points.empty() && *tick < lane.points.back().tick)
                error = at + "points go in order of their ticks";

            if (error.isNotEmpty())
                return std::nullopt;

            lane.points.push_back ({ *tick, (float) value });
        }

        return lane;
    }

    juce::var SequenceTools::writeSequence (const juce::var& a)
    {
        auto length = sequence.length;
        auto loop = sequence.loopLength;

        if (! a["length"].isVoid())
        {
            const auto l = wholeNumber (a["length"]);

            if (! l || *l < ticksPerBeat || *l % ticksPerBeat != 0 || *l > (juce::int64) ticksPerBeat * 16 * 256)
                return refused ("length is a whole number of beats in ticks (960 a beat), from one beat to 256 bars");

            length = *l;
        }

        if (! a["loop_length"].isVoid())
        {
            const auto l = wholeNumber (a["loop_length"]);

            if (! l || *l < 0 || *l > length)
                return refused ("loop_length is ticks from 0 (no loop) to the length, " + juce::String (length));

            loop = *l;
        }

        auto key = sequence.key;

        if (! a["key"].isVoid())
        {
            const auto k = a["key"].toString().trim();

            if (k.isNotEmpty() && ! NoteTools::parseKey (k))
                return refused (k.quoted() + " isn't a key: give a pitch and a scale, such as \"A minor\" or \"Bb dorian\"");

            key = k.isNotEmpty() ? NoteTools::parseKey (k)->name() : juce::String();
        }

        const auto* notes = a["notes"].getArray();

        if (notes == nullptr || notes->size() > maxNotes)
            return refused ("notes is a list of up to " + juce::String (maxNotes) + " notes, each with pitch, start and length in ticks");

        // Checked against the length and loop as they will be.
        const auto keptLoop = sequence.loopLength;
        sequence.loopLength = loop;
        std::vector<SequenceFile::Note> read;
        juce::String error;

        for (int i = 0; i < notes->size() && error.isEmpty(); ++i)
            if (const auto n = readNote ((*notes)[i], length, "note " + juce::String (i + 1) + ": ", error))
                read.push_back (*n);

        sequence.loopLength = keptLoop;

        if (error.isNotEmpty())
            return refused (error);

        std::vector<SequenceFile::Lane> lanes = sequence.lanes;

        if (! a["lanes"].isVoid())
        {
            const auto* given = a["lanes"].getArray();

            if (given == nullptr)
                return refused ("lanes is a list of lanes, each with controller and points");

            lanes.clear();

            for (int i = 0; i < given->size(); ++i)
            {
                const auto lane = readLane ((*given)[i], length, "lane " + juce::String (i + 1) + ": ", error);

                if (! lane)
                    return refused (error);

                for (auto& l : lanes)
                    if (l.controller == lane->controller)
                        return refused ("lane " + juce::String (i + 1) + ": there is already a lane for " + ControllerLane::nameOf (lane->controller));

                lanes.push_back (*lane);
            }
        }
        else
        {
            // Lanes kept: none may reach past a shorter length.
            for (auto& l : lanes)
                for (auto& p : l.points)
                    if (p.tick > length)
                        return refused ("the " + ControllerLane::nameOf (l.controller) + " lane goes past the new length: give the lanes too");
        }

        sequence.notes = std::move (read);
        sequence.lanes = std::move (lanes);
        sequence.length = length;
        sequence.loopLength = loop;
        sequence.key = key;
        wrote = true;

        const auto count = (int) sequence.notes.size();
        juce::String step = "Write " + juce::String (count) + (count == 1 ? " note" : " notes");

        if (! sequence.lanes.empty())
            step << " and " << (int) sequence.lanes.size() << (sequence.lanes.size() == 1 ? " lane" : " lanes");

        step << " into " << sequence.name.quoted() << ", " << ticksText (length);

        if (key.isNotEmpty())
            step << " in " << key;

        steps.add (step);
        return object ({ { "ok", true }, { "step", step }, { "outside_key", notesOutsideKey (sequence) } });
    }

    //==============================================================================
    juce::var SequenceTools::suggest (const juce::var& a)
    {
        const auto text = a["review"].toString().trim();
        const auto* list = a["suggestions"].getArray();

        if (text.isEmpty())
            return refused ("review is a few sentences on the sequence");

        if (list == nullptr || list->size() > maxSuggestions)
            return refused ("suggestions is a list of up to " + juce::String (maxSuggestions) + " suggestions");

        std::vector<Suggestion> made;
        const auto length = sequence.length;

        for (int i = 0; i < list->size(); ++i)
        {
            const auto& s = (*list)[i];
            const auto which = "suggestion " + juce::String (i + 1) + ": ";
            Suggestion out;
            out.title = s["title"].toString().trim();
            out.why = s["why"].toString().trim();
            const auto kind = s["kind"].toString().trim().toLowerCase();
            juce::String error;

            if (out.title.isEmpty())
                return refused (which + "give it a title");

            if (kind == "notes")
            {
                out.kind = Suggestion::Kind::notes;
                auto notes = sequence.notes;

                if (const auto* given = s["remove"].getArray())
                    for (auto& r : *given)
                    {
                        const auto pitch = wholeNumber (r["pitch"]), start = wholeNumber (r["start"]);
                        const auto found = pitch && start ? findNote (notes, (int) *pitch, *start) : notes.end();

                        if (found == notes.end())
                            return refused (which + "there is no note to take out at pitch " + r["pitch"].toString() + ", tick " + r["start"].toString()
                                            + ": name each note by its pitch and start from get_sequence");

                        out.remove.push_back (*found);
                        notes.erase (found);
                    }

                if (const auto* given = s["change"].getArray())
                    for (auto& c : *given)
                    {
                        const auto pitch = wholeNumber (c["pitch"]), start = wholeNumber (c["start"]);
                        const auto found = pitch && start ? findNote (notes, (int) *pitch, *start) : notes.end();

                        if (found == notes.end())
                            return refused (which + "there is no note to change at pitch " + c["pitch"].toString() + ", tick " + c["start"].toString()
                                            + ": name each note by its pitch and start from get_sequence");

                        if (! c["to"].isObject())
                            return refused (which + "each change has \"to\": what changes (pitch, start, length, velocity, probability)");

                        // The note as it is, with what changes.
                        auto merged = juce::var (new juce::DynamicObject());
                        auto* m = merged.getDynamicObject();
                        m->setProperty ("pitch", found->pitch);
                        m->setProperty ("start", found->start);
                        m->setProperty ("length", found->length);
                        m->setProperty ("velocity", (double) found->velocity * 127.0);
                        m->setProperty ("probability", found->probability);

                        for (auto& p : c["to"].getDynamicObject()->getProperties())
                            m->setProperty (p.name, p.value);

                        const auto to = readNote (merged, length, which, error);

                        if (! to)
                            return refused (error);

                        // A velocity left alone stays exactly as it was.
                        auto changed = *to;

                        if (! c["to"].hasProperty ("velocity"))
                            changed.velocity = found->velocity;

                        out.change.push_back ({ *found, changed });
                        notes.erase (found);
                    }

                if (const auto* given = s["add"].getArray())
                    for (int n = 0; n < given->size(); ++n)
                    {
                        const auto note = readNote ((*given)[n], length, which + "note " + juce::String (n + 1) + " to add: ", error);

                        if (! note)
                            return refused (error);

                        out.add.push_back (*note);
                    }

                if (out.remove.empty() && out.change.empty() && out.add.empty())
                    return refused (which + "a notes suggestion removes, changes or adds notes");
            }
            else if (kind == "lanes")
            {
                out.kind = Suggestion::Kind::lanes;
                const auto* given = s["lanes"].getArray();

                if (given == nullptr || given->isEmpty())
                    return refused (which + "a lanes suggestion has lanes, each with controller and points");

                for (int l = 0; l < given->size(); ++l)
                {
                    const auto lane = readLane ((*given)[l], length, which + "lane " + juce::String (l + 1) + ": ", error);

                    if (! lane)
                        return refused (error);

                    out.lanes.push_back (*lane);
                }
            }
            else if (kind == "effect")
            {
                out.kind = Suggestion::Kind::effect;
                const auto name = s["effect"].toString().trim();

                if (name.isEmpty())
                    return refused (which + "an effect suggestion names the effect, by its id or name from list_plugins");

                if (findEffect == nullptr)
                    return refused (which + "no plug-ins are known here, so no effect can be suggested");

                const auto found = findEffect (name, error);

                if (! found)
                    return refused (which + error);

                if (found->isInstrument)
                    return refused (which + found->name + " is an instrument, not an effect");

                out.effect = *found;
            }
            else if (kind == "automation")
            {
                out.kind = Suggestion::Kind::automation;
                out.parameter = s["parameter"].toString().trim();

                if (context.instrument.isEmpty())
                    return refused (which + "the voice has no instrument to automate");

                if (out.parameter.isEmpty())
                    return refused (which + "an automation suggestion names the instrument's parameter");

                if (! context.instrumentParameters.isEmpty())
                {
                    const auto index = context.instrumentParameters.indexOf (out.parameter, true);

                    if (index < 0)
                        return refused (which + context.instrument + " has no parameter called " + out.parameter.quoted()
                                        + "; get_sequence lists its parameters");

                    out.parameter = context.instrumentParameters[index];
                }

                juce::var lane (new juce::DynamicObject());
                lane.getDynamicObject()->setProperty ("controller", 1);
                lane.getDynamicObject()->setProperty ("points", s["points"]);
                const auto read = readLane (lane, length, which, error);

                if (! read)
                    return refused (error);

                out.points = read->points;
            }
            else
                return refused (which + "kind is notes, lanes, effect or automation");

            made.push_back (std::move (out));
        }

        review = text;
        suggestions = std::move (made);

        juce::Array<juce::var> listed;

        for (size_t i = 0; i < suggestions.size(); ++i)
            listed.add (object ({ { "number", (int) i + 1 }, { "title", suggestions[i].title }, { "kind", kindName (suggestions[i].kind) },
                                  { "applies", suggestions[i].canApply() } }));

        return object ({ { "ok", true },
                         { "shown", "The review and suggestions are shown to the user, each with Apply, and Apply All. Nothing changes until they apply one." },
                         { "outside_key", notesOutsideKey (sequence) },
                         { "suggestions", listed } });
    }

    //==============================================================================
    std::optional<SequenceFile::Sequence> SequenceTools::apply (const SequenceFile::Sequence& in, const std::vector<Suggestion>& chosen, juce::String& error)
    {
        auto q = in;

        for (auto& s : chosen)
        {
            switch (s.kind)
            {
                case Suggestion::Kind::notes:
                {
                    for (auto& r : s.remove)
                    {
                        const auto found = findNote (q.notes, r.pitch, r.start);

                        if (found == q.notes.end())
                        {
                            error = s.title.quoted() + ": the " + noteName (r.pitch) + " at " + placeOf (r.start, q.beatsPerBar) + " it takes out isn't there now.";
                            return std::nullopt;
                        }

                        q.notes.erase (found);
                    }

                    for (auto& [from, to] : s.change)
                    {
                        const auto found = findNote (q.notes, from.pitch, from.start);

                        if (found == q.notes.end())
                        {
                            error = s.title.quoted() + ": the " + noteName (from.pitch) + " at " + placeOf (from.start, q.beatsPerBar) + " it changes isn't there now.";
                            return std::nullopt;
                        }

                        *found = to;
                    }

                    for (auto& n : s.add)
                        q.notes.push_back (n);

                    std::stable_sort (q.notes.begin(), q.notes.end(), [] (const SequenceFile::Note& x, const SequenceFile::Note& y) { return x.start < y.start; });
                    break;
                }

                case Suggestion::Kind::lanes:
                    for (auto& lane : s.lanes)
                    {
                        const auto found = std::find_if (q.lanes.begin(), q.lanes.end(), [&] (const SequenceFile::Lane& l) { return l.controller == lane.controller; });

                        if (found != q.lanes.end())
                            *found = lane;
                        else
                            q.lanes.push_back (lane);
                    }

                    break;

                case Suggestion::Kind::effect:
                {
                    if (q.voice == nullptr)
                    {
                        error = s.title.quoted() + ": the sequence has no voice to add " + s.effect.name + " to.";
                        return std::nullopt;
                    }

                    // As a track's chain is written: a SLOT holding the plug-in, with no STATE (its defaults).
                    auto voice = std::make_shared<juce::XmlElement> (*q.voice);
                    auto* effects = voice->getChildByName ("EFFECTS");

                    if (effects == nullptr)
                        effects = voice->createNewChildElement ("EFFECTS");

                    effects->createNewChildElement ("SLOT")->addChildElement (s.effect.createXml().release());
                    q.voice = std::move (voice);
                    break;
                }

                case Suggestion::Kind::automation:
                    break;
            }
        }

        return q;
    }

    //==============================================================================
    namespace SequencerLink
    {
        SequenceTools::Context contextFor (SequencerView& view, const PluginCatalogue* catalogue, double sessionTempo)
        {
            SequenceTools::Context c;
            c.sequence = view.currentSequence();
            c.tempo = view.getTempo();
            c.sessionTempo = sessionTempo;

            const auto& chain = view.getPlayer().getEffects();
            const auto track = view.getTrackId();
            const auto* instrument = chain.getInstrument (track);

            if (instrument != nullptr)
            {
                c.instrument = instrument->getName();

                if (catalogue != nullptr)
                    c.instrumentAbout = catalogue->summaryOf (instrument->getPluginDescription());

                for (auto* p : instrument->getParameters())
                    if (c.instrumentParameters.size() < maxParameters)
                        if (const auto name = p->getName (64).trim(); name.isNotEmpty())
                            c.instrumentParameters.addIfNotAlreadyThere (name);
            }
            else
                c.instrument = SequenceFile::instrumentOf (c.sequence);   // still loading, or not loaded

            for (int i = instrument != nullptr ? 1 : 0; i < chain.getNumSlots (track); ++i)
                if (auto* p = chain.getPlugin (track, i))
                    c.effects.add (p->getName());

            return c;
        }

        juce::String applyPlan (SequencerView& view, const Workspace& w, const juce::String& request)
        {
            const auto* tools = w.getSequenceTools();

            if (tools == nullptr || ! tools->written())
                return {};

            if (! sameMusic (view.getDocument().get(), tools->getContext().sequence))
                return "The sequence in the Sequencer changed while the plan was made, so it wasn't applied. Ask again.";

            const auto& written = tools->getSequence();
            auto q = view.currentSequence();
            q.notes = written.notes;
            q.lanes = written.lanes;
            q.length = written.length;
            q.loopLength = written.loopLength;
            q.key = written.key;

            const auto words = request.length() > 40 ? request.substring (0, 40).trimEnd() + "..." : request;
            view.applyEdit (q, "Ask Torq: " + words);
            return {};
        }

        juce::String applySuggestions (SequencerView& view, const Workspace& w, const std::vector<int>& which)
        {
            const auto* tools = w.getSequenceTools();

            if (tools == nullptr)
                return "There are no suggestions for a sequence.";

            const auto& all = tools->getSuggestions();
            std::vector<SequenceTools::Suggestion> chosen;

            for (auto i : which)
                if (i >= 0 && i < (int) all.size() && all[(size_t) i].canApply())
                    chosen.push_back (all[(size_t) i]);

            if (chosen.empty())
                return "None of those suggestions can be applied.";

            juce::String error;
            const auto edited = SequenceTools::apply (view.currentSequence(), chosen, error);

            if (! edited)
                return error;

            view.applyEdit (*edited, "Ask Torq: " + (chosen.size() == 1 ? chosen.front().title : juce::String ((int) chosen.size()) + " suggestions"));
            return {};
        }
    }
}
