#pragma once

#include <JuceHeader.h>
#include "SequenceFile.h"

class SequencerView;
class PluginCatalogue;

/** Experimental: the tools a language model writes and reviews the Sequencer's sequence with, on
    a working copy of it, in ticks (SequenceFile::ticksPerBeat to a beat).

    write_sequence puts notes and lanes into the copy, as a step of the plan; once the plan is
    applied, the copy goes into the Sequencer as one undo step of its own history.
    suggest_for_sequence keeps a review and a list of suggestions, each checked against the copy:
    edits to the notes, controller lanes, an effect for the voice, or automation of the
    instrument's parameters. The first three apply in the Sequencer, one at a time or all together,
    each as one undo step there. Automation can't be kept in a sequence yet (a .tqseq holds notes,
    lanes and a voice without automation), so an automation suggestion is advice only: it says
    what to draw once the sequence is on a track in a session. */
namespace Ai
{
    class SequenceTools
    {
    public:
        /** What Ask Torq is told of the sequence and where it plays. */
        struct Context
        {
            SequenceFile::Sequence sequence;          // with the voice as it is loaded now
            juce::String instrument;                  // the voice's instrument, or empty with none
            juce::String instrumentAbout;             // what the plug-in catalogue says it is, or empty
            juce::StringArray instrumentParameters;   // its parameters' names, when it is loaded
            juce::StringArray effects;                // the voice's effects, in order
            double tempo = 120.0;                     // the Sequencer's tempo
            double sessionTempo = 0.0;                // the open session's, or 0 with none
        };

        /** A suggestion from a review: one kind of change, with its title and reason. */
        struct Suggestion
        {
            enum class Kind { notes, lanes, effect, automation };

            Kind kind = Kind::notes;
            juce::String title, why;

            // notes: the notes to take out and to change (each found by its pitch and start), and to add.
            std::vector<SequenceFile::Note> remove, add;
            std::vector<std::pair<SequenceFile::Note, SequenceFile::Note>> change;   // from, to

            // lanes: each replaces the lane of its controller, or is added.
            std::vector<SequenceFile::Lane> lanes;

            // effect: added at the end of the voice's chain, with its default settings.
            juce::PluginDescription effect;

            // automation: an instrument parameter's values (0 to 1) at ticks. Advice only.
            juce::String parameter;
            std::vector<SequenceFile::LanePoint> points;

            bool canApply() const noexcept    { return kind != Kind::automation; }

            /** One line for the list: its title, and what it changes. */
            juce::String describe (int beatsPerBar) const;
        };

        /** An effect plug-in by its id or name, or nothing with the reason in error. */
        using FindEffect = std::function<std::optional<juce::PluginDescription> (const juce::String&, juce::String& error)>;

        /** steps: the plan, which write_sequence adds its line to. */
        SequenceTools (Context, juce::StringArray& steps, FindEffect findEffect = nullptr);

        /** The tools, as entries for the Chat Completions "tools" array. */
        static juce::Array<juce::var> toolDefinitions();

        /** What the model is told about sequences (the system message's own part). */
        static juce::String instructions();

        /** Runs the tool when it is one of these and returns its result; nothing when it isn't.
            A bad call comes back as {"ok": false, "error": ...} for the model to correct. */
        std::optional<juce::var> run (const juce::String& tool, const juce::var& args);

        const Context& getContext() const noexcept                      { return context; }

        /** The working copy, and whether write_sequence changed it. */
        const SequenceFile::Sequence& getSequence() const noexcept      { return sequence; }
        bool written() const noexcept                                   { return wrote; }

        /** The latest review, its suggestions, and the notes outside the key it names. */
        const juce::String& getReview() const noexcept                  { return review; }
        const std::vector<Suggestion>& getSuggestions() const noexcept  { return suggestions; }
        juce::StringArray getOutsideKey() const                         { return notesOutsideKey (sequence); }

        //==============================================================================
        /** The sequence with these suggestions applied in order (automation ones are skipped), or
            nothing with the reason: a note to change that is no longer there, an effect with no voice. */
        static std::optional<SequenceFile::Sequence> apply (const SequenceFile::Sequence&, const std::vector<Suggestion>&, juce::String& error);

        /** Each note outside the sequence's key, named with where it is: "F#4 at bar 1, beat 2.5".
            Empty when it has no key, or a key that can't be read. */
        static juce::StringArray notesOutsideKey (const SequenceFile::Sequence&);

        /** A tick as a place in the bar: "bar 2, beat 1.5". */
        static juce::String placeOf (juce::int64 tick, int beatsPerBar);

        /** A pitch's name, middle C (60) being C4. */
        static juce::String noteName (int pitch);

        /** The sequence as get_sequence gives it, with the context. Public for the tests. */
        static juce::var describe (const Context&, const SequenceFile::Sequence&);

    private:
        juce::var getSequenceTool();
        juce::var writeSequence (const juce::var&);
        juce::var suggest (const juce::var&);

        std::optional<SequenceFile::Note> readNote (const juce::var&, juce::int64 length, const juce::String& which, juce::String& error) const;
        std::optional<SequenceFile::Lane> readLane (const juce::var&, juce::int64 length, const juce::String& which, juce::String& error) const;

        Context context;
        SequenceFile::Sequence sequence;
        juce::StringArray& steps;
        FindEffect findEffect;
        bool wrote = false;
        juce::String review;
        std::vector<Suggestion> suggestions;
    };

    class Workspace;

    /** Where Ask Torq meets the Sequencer window (on the message thread). */
    namespace SequencerLink
    {
        /** What Ask Torq is told: the Sequencer's sequence, its voice, and the session's tempo
            (0 with no session open). */
        SequenceTools::Context contextFor (SequencerView&, const PluginCatalogue*, double sessionTempo);

        /** The written sequence into the Sequencer as one undo step. An error when the sequence
            there changed since the request was made, or it is another one now. */
        juce::String applyPlan (SequencerView&, const Workspace&, const juce::String& request);

        /** Suggestions (by their index in the review) into the Sequencer as one undo step. */
        juce::String applySuggestions (SequencerView&, const Workspace&, const std::vector<int>& which);
    }
}
