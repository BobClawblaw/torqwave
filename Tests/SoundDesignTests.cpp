// Exact values are what the session holds.
#pragma clang diagnostic ignored "-Wfloat-equal"

#include <JuceHeader.h>
#include "../Source/AudioEdits.h"
#include "../Source/AudioEngine.h"
#include "../Source/AudioFileIO.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/ClipEffects.h"
#include "../Source/ClipEffectsPanel.h"
#include "../Source/MultitrackView.h"
#include "../Source/OfflineRender.h"
#include "../Source/OpenFiles.h"
#include "../Source/Session.h"
#include "../Source/SessionMixer.h"
#include "../Source/SoundCategories.h"
#include "../Source/SoundDesign.h"
#include "../Source/SoundLibrary.h"
#include "../Source/SoundMetadata.h"
#include "../Source/TempoFollow.h"
#include "TestUtils.h"
#include "SoundDesignTests.h"
#include <thread>

namespace
{
    using TestUtils::check;

    constexpr double rate = 48000.0;

    /** A stereo tone that swells and fades, the channels a little apart. */
    AudioDocument::Snapshot tone (int frames, double hz)
    {
        juce::AudioBuffer<float> b (2, frames);

        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < frames; ++i)
                b.setSample (ch, i, (float) (0.5 * std::sin (juce::MathConstants<double>::twoPi * hz * (1.0 + 0.01 * ch) * i / rate)
                                             * std::sin (juce::MathConstants<double>::pi * i / frames)));

        return AudioEdits::fromBuffer (std::move (b));
    }

    juce::AudioBuffer<float> bufferOf (const AudioDocument::Snapshot& s, juce::int64 from = 0, juce::int64 frames = -1)
    {
        const auto n = (int) (frames < 0 ? s->getNumFrames() - from : frames);
        juce::AudioBuffer<float> b (s->getNumChannels(), n);
        s->read (b, 0, from, n);
        return b;
    }

    bool same (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return false;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            if (std::memcmp (a.getReadPointer (ch), b.getReadPointer (ch), sizeof (float) * (size_t) a.getNumSamples()) != 0)
                return false;

        return true;
    }

    /** The biggest difference between two buffers of the same shape (a large number otherwise). */
    float worst (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
    {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
            return 1.0e9f;

        auto w = 0.0f;

        for (int ch = 0; ch < a.getNumChannels(); ++ch)
            for (int i = 0; i < a.getNumSamples(); ++i)
                w = juce::jmax (w, std::abs (a.getSample (ch, i) - b.getSample (ch, i)));

        return w;
    }

    const SessionClip& clipOf (const SessionState& s, juce::uint32 id)
    {
        const auto where = s.findClip (id);
        jassert (where.has_value());
        return s.tracks[(size_t) where->first].clips[(size_t) where->second];
    }

    SessionClip& clipOf (SessionState& s, juce::uint32 id)
    {
        const auto where = s.findClip (id);
        jassert (where.has_value());
        return s.tracks[(size_t) where->first].clips[(size_t) where->second];
    }

    /** Session frames [0, frames) through the mixer, as playback reads them, in blocks of `block`. */
    juce::AudioBuffer<float> play (const SessionState& s, int frames, int block)
    {
        SessionMixer::Buffers buffers;
        buffers.reserve (block);
        juce::AudioBuffer<float> out (2, frames);

        for (int at = 0; at < frames; at += block)
            SessionMixer::render (s, at, out, at, juce::jmin (block, frames - at), buffers);

        return out;
    }

    /** A mixdown of the range, as Mix Down writes it. */
    juce::AudioBuffer<float> mixDown (const SessionState& s, juce::Range<juce::int64> range)
    {
        juce::AudioBuffer<float> out;
        OfflineRender::mixSession (s, range, out, nullptr, {});
        return out;
    }

    /** Runs work on a thread of its own, the message loop running meanwhile (plug-ins are handed
        back to it), as the app's background jobs do. */
    void onWorker (std::function<void()> work)
    {
        std::atomic<bool> done { false };
        std::thread worker ([&] { work(); done = true; });
        TestUtils::runUntil ([&] { return done.load(); }, 120000);
        worker.join();
    }

    juce::PluginDescription builtIn (const juce::String& name)
    {
        for (auto& d : BuiltInEffects::getDescriptions())
            if (d.name == name)
                return d;

        jassertfalse;
        return {};
    }

    juce::File freshFolder (const juce::String& name)
    {
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (name);
        dir.deleteRecursively();
        dir.createDirectory();
        return dir;
    }

    //==============================================================================
    void testEditInPlace()
    {
        // One file on two tracks: clip A trimmed to 0.25-1.25 s of it at 1 s, clip B the whole of it.
        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        s.addTrack();
        const auto source = tone (96000, 220.0);
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Kick.wav");
        const auto a = SessionEdits::addClip (s, 0, file, source, 48000);
        const auto b = SessionEdits::addClip (s, 1, file, source, 0);
        clipOf (s, a).offset = 12000;
        clipOf (s, a).length = 48000;
        clipOf (s, a).name = "Kick";

        Session session;
        session.reset (s, {});
        const auto edit = SoundDesign::editClip (*session.getState(), a);
        check (edit.audio != nullptr && edit.error.isEmpty() && ! edit.original && edit.clipName == "Kick"
                   && same (bufferOf (edit.audio), bufferOf (source, 12000, 48000)),
               "edit in place: a clip opens as the part of its file it plays");

        // Edited in the editor: half as loud, and cut to 0.75 s.
        auto changed = bufferOf (edit.audio, 0, 36000);
        changed.applyGain (0.5f);
        const auto edited = AudioEdits::fromBuffer (juce::AudioBuffer<float> (changed));
        juce::String error;
        const auto saved = SoundDesign::toSave (edit, edited, error);
        auto next = *session.getState();
        const auto editFile = file.getSiblingFile ("Kick Edit.wav");
        const auto put = saved == edited && SoundDesign::putBack (next, edit, editFile, saved, error);
        session.apply (std::move (next), "Edit in Place");

        const auto after = session.getState();
        const auto& now = clipOf (*after, a);
        const auto& other = clipOf (*after, b);
        check (put && error.isEmpty() && now.takes.size() == 2 && now.activeTake == 1 && now.audio == saved && now.source == editFile
                   && now.takes[0].audio == source && now.takes[0].source == file && now.position == 48000 && now.length == 36000 && now.offset == 0
                   && same (bufferOf (SessionEdits::playedPart (now, now.audio)), changed),
               "edit in place: saving makes the edit a new take in use, from where the clip starts, as long as the edit");
        check (other.audio == source && other.offset == 0 && other.length == 96000 && other.takes.empty() && other.source == file,
               "edit in place: another clip of the same file is untouched");

        // What it played is a take away.
        auto back = *after;
        const auto chosen = SessionEdits::chooseTake (back, a, 0);
        const auto& first = clipOf (back, a);
        check (chosen && first.audio == source && same (bufferOf (SessionEdits::playedPart (first, first.audio)), bufferOf (source, 12000, 36000)),
               "edit in place: choosing the first take plays what the clip played before, from the same frame");

        session.undo();
        const auto& undone = clipOf (*session.getState(), a);
        const auto undid = undone.audio == source && undone.takes.empty() && undone.offset == 12000 && undone.length == 48000;
        session.redo();
        check (undid && clipOf (*session.getState(), a).audio == saved,
               "edit in place: one undo step puts the clip back as it was; redo brings the edit back");

        // A clip that is locked, or on a frozen track, isn't opened.
        auto locked = *session.getState();
        clipOf (locked, a).locked = true;
        auto frozen = *session.getState();
        frozen.tracks[1].frozenAudio = source;
        check (SoundDesign::editClip (locked, a).audio == nullptr && SoundDesign::editClip (locked, a).error.contains ("locked")
                   && SoundDesign::editClip (frozen, b).audio == nullptr && SoundDesign::editClip (frozen, b).error.contains ("frozen")
                   && ! SessionEdits::addEditedTake (locked, a, editFile, edited),
               "edit in place: a locked clip and a frozen track's clip are refused, saying why");

        // A comp: what it plays opens, and saving ends the comp, its takes staying.
        auto comped = *session.getState();
        auto& c = clipOf (comped, a);
        c.comp = { { 0, 0 }, { c.takeZeroFrame (c.position + 18000), 1 } };
        const auto compEdit = SoundDesign::editClip (comped, a);
        const auto compPlays = compEdit.audio != nullptr && same (bufferOf (compEdit.audio), bufferOf (SessionEdits::compAudio (c)));
        const auto compPut = SoundDesign::putBack (comped, compEdit, editFile, compEdit.audio, error);
        const auto& afterComp = clipOf (comped, a);
        check (compPlays && compPut && afterComp.comp.empty() && afterComp.takes.size() == 3 && afterComp.activeTake == 2,
               "edit in place: a comped clip opens as it plays; its edit is a third take, the comp ended");
    }

    void testFollowingEdit()
    {
        // A clip following the tempo plays 0.5-1.5 s of its file; another clip plays all of it.
        SessionState s;
        s.sampleRate = rate;
        s.tempo = 120.0;
        s.addTrack();
        s.addTrack();
        const auto source = tone (96000, 330.0);
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Loop.wav");
        const auto id = SessionEdits::addClip (s, 0, file, source, 0);
        const auto other = SessionEdits::addClip (s, 1, file, source, 0);
        clipOf (s, id).offset = 24000;
        clipOf (s, id).length = 48000;
        SessionEdits::setClipTempo (s, id, 100.0);
        TempoFollow::renderAll (s);
        const auto lengthBefore = clipOf (s, id).length;

        const auto edit = SoundDesign::editClip (s, id);
        const auto opened = edit.audio != nullptr && edit.original && edit.from == source && std::abs (edit.start - 24000) <= 1
                            && std::abs (edit.audio->getNumFrames() - 48000) <= 2
                            && same (bufferOf (edit.audio), bufferOf (source, edit.start, edit.audio->getNumFrames()));

        juce::String error;
        const auto shorter = SoundDesign::toSave (edit, AudioEdits::fromBuffer (bufferOf (edit.audio, 0, 1000)), error);
        const auto refusedSaying = shorter == nullptr && error.contains ("follows the tempo");

        auto changed = bufferOf (edit.audio);
        changed.applyGain (0.25f);
        const auto whole = SoundDesign::toSave (edit, AudioEdits::fromBuffer (juce::AudioBuffer<float> (changed)), error);
        const auto spliced = whole != nullptr && whole->getNumFrames() == source->getNumFrames()
                             && same (bufferOf (whole, 0, edit.start), bufferOf (source, 0, edit.start))
                             && same (bufferOf (whole, edit.start, changed.getNumSamples()), changed)
                             && same (bufferOf (whole, edit.start + changed.getNumSamples()), bufferOf (source, edit.start + changed.getNumSamples()));
        const auto put = SoundDesign::putBack (s, edit, file.getSiblingFile ("Loop Edit.wav"), whole, error);
        const auto& c = clipOf (s, id);
        const auto stale = TempoFollow::isStale (s, c);
        TempoFollow::renderAll (s);

        check (opened && refusedSaying && spliced && put && c.tempoFollow.original == whole && stale && clipOf (s, id).length == lengthBefore
                   && clipOf (s, id).takes.empty() && clipOf (s, other).audio == source,
               "edit in place: a clip following the tempo opens the part of its original it plays; an edit of another length is refused, "
               "saying why; one as long goes into the original, stretched again, the clip as long as before and the other clip untouched");

        // The original changed in the session since it was opened: refused.
        check (! SoundDesign::putBack (s, edit, file, whole, error) && error.contains ("changed"),
               "edit in place: an edit opened from an original that has since changed is refused");
    }

    //==============================================================================
    /** A session with a clip at 0.1 s playing 1.25 s of a tone, with fades and gain, shifted down
        three semitones through a reverb and a compressor; another track's clip with none. */
    struct WithEffects
    {
        SessionState s;
        juce::uint32 id = 0, plain = 0;
        juce::String chain;
    };

    WithEffects withEffects()
    {
        WithEffects w;
        w.s.sampleRate = rate;
        w.s.addTrack();
        w.s.addTrack();
        w.id = SessionEdits::addClip (w.s, 0, juce::File ("/takes/voice.wav"), tone (72000, 440.0), 4800);
        w.plain = SessionEdits::addClip (w.s, 1, juce::File ("/takes/bed.wav"), tone (96000, 110.0), 0);
        auto& c = clipOf (w.s, w.id);
        c.name = "Voice";
        c.offset = 1000;
        c.length = 60000;
        c.fadeIn = 2400;
        c.fadeOut = 9600;
        c.gainDb = -3.0f;
        w.chain = ClipEffects::withSlot (ClipEffects::withSlot ({}, builtIn ("Reverb")), builtIn ("Compressor"));
        SessionEdits::setClipEffects (w.s, w.id, -3.0, w.chain);
        return w;
    }

    void testClipEffects (AudioEngine& engine)
    {
        auto w = withEffects();
        auto& s = w.s;
        const auto staleFirst = ClipEffects::isStale (s, clipOf (s, w.id));
        const auto dryFirst = play (s, 72000, 512);
        juce::String errors;
        onWorker ([&] { errors = ClipEffects::renderAll (s, engine.pluginFormatManager); });
        const auto& c = clipOf (s, w.id);
        const auto dry = bufferOf (SessionEdits::playedPart (c, c.audio));
        check (staleFirst && errors.isEmpty() && c.effects.rendered != nullptr && c.effects.rendered->getNumFrames() == c.length
                   && ! ClipEffects::isStale (s, c) && ! same (bufferOf (c.effects.rendered), dry) && clipOf (s, w.plain).effects.rendered == nullptr,
               "clip effects: a clip's pitch shift and chain render as long as the clip; a clip without effects isn't rendered");

        // Played in blocks of any size, and mixed down, the same samples.
        constexpr int frames = 4800 + 60000 + 7200;
        const auto reference = play (s, frames, 512);
        auto allSizes = true;
        juce::StringArray sizes;

        for (auto block : { 1, 7, 64, 333, 4096 })
        {
            allSizes = allSizes && same (play (s, frames, block), reference);
            sizes.add (juce::String (block));
        }

        const auto mixed = mixDown (s, { 0, frames });
        check (allSizes && same (mixed, reference) && ! same (reference, dryFirst),
               "clip effects: playback in blocks of " + sizes.joinIntoString (", ") + " and the mixdown are identical, sample for sample");

        // What is heard is the render, with the clip's fades and gain on it, as a plain clip of that audio.
        auto plainCopy = s;
        auto& p = clipOf (plainCopy, w.id);
        p.audio = c.effects.rendered;
        p.offset = 0;
        p.effects = {};
        check (same (play (plainCopy, frames, 512), reference),
               "clip effects: the clip plays its render under its fades and gain, as a clip of that audio would");

        // With every slot bypassed and no shift, the render is the dry audio, bit for bit.
        auto bypassed = s;
        SessionEdits::setClipEffects (bypassed, w.id, 0.0, ClipEffects::withBypass (ClipEffects::withBypass (w.chain, 0, true), 1, true));
        onWorker ([&] { errors = ClipEffects::renderAll (bypassed, engine.pluginFormatManager); });
        check (errors.isEmpty() && same (bufferOf (clipOf (bypassed, w.id).effects.rendered), dry),
               "clip effects: with every effect bypassed and no pitch shift the render is the clip's audio, bit for bit");

        // A change that makes the render stale drops it (the clip plays dry until it is rendered again);
        // undo brings back the state that had it.
        Session session;
        session.reset (s, {});
        const auto kept = clipOf (*session.getState(), w.id).effects.rendered == c.effects.rendered;
        auto trimmed = *session.getState();
        clipOf (trimmed, w.id).length -= 4800;
        session.apply (std::move (trimmed), "Trim");
        const auto dropped = clipOf (*session.getState(), w.id).effects.rendered == nullptr && ClipEffects::isStale (*session.getState(), clipOf (*session.getState(), w.id));
        auto offState = *session.getState();
        const auto off = SessionEdits::setClipEffects (offState, w.id, 0.0, {}) && ! clipOf (offState, w.id).effects.isOn();
        session.undo();
        check (kept && dropped && off && clipOf (*session.getState(), w.id).effects.rendered == c.effects.rendered,
               "clip effects: a trim drops the render and the clip waits to be rendered again; undo brings the render back; effects can be taken off");

        // Out of range or not a rack: refused.
        auto refused = s;
        check (! SessionEdits::setClipEffects (refused, w.id, 12.5, w.chain) && ! SessionEdits::setClipEffects (refused, w.id, 0.0, "<NOTARACK/>")
                   && ! SessionEdits::setClipEffects (refused, w.id, -3.0, w.chain),
               "clip effects: a shift past an octave, a chain that isn't a rack and no change are refused");
    }

    void testRenderer (AudioEngine& engine)
    {
        auto w = withEffects();
        auto reference = w.s;
        onWorker ([&] { ClipEffects::renderAll (reference, engine.pluginFormatManager); });

        Session session;
        ClipEffects::Renderer renderer (session, engine.pluginFormatManager);
        juce::StringArray failures;
        renderer.onFailed = [&failures] (const juce::String& clip, const juce::String& why) { failures.add (clip + ": " + why); };
        auto doneCalls = 0;
        session.reset (w.s, {});
        renderer.whenDone ([&doneCalls] { ++doneCalls; });
        const auto started = juce::Time::getMillisecondCounterHiRes();
        const auto finished = TestUtils::runUntil ([&] { return renderer.isUpToDate(); }, 60000);
        const auto took = juce::Time::getMillisecondCounterHiRes() - started;
        TestUtils::runUntil ([&] { return doneCalls > 0; }, 1000);
        const auto& rendered = clipOf (*session.getState(), w.id).effects.rendered;
        check (finished && doneCalls == 1 && rendered != nullptr && same (bufferOf (rendered), bufferOf (clipOf (reference, w.id).effects.rendered))
                   && ! session.canUndo() && ! session.isDirty(),
               "clip effects renderer: renders the clip in the background (" + juce::String (took, 0) + " ms), bit for bit what a render on its own gives, "
               "put in place (no undo step, nothing to save)");

        auto next = *session.getState();
        SessionEdits::setClipEffects (next, w.id, -3.0, ClipEffects::withoutSlot (w.chain, 1));
        session.apply (std::move (next), "Remove Clip Effect");
        const auto waits = ! renderer.isUpToDate();
        const auto again = TestUtils::runUntil ([&] { return renderer.isUpToDate(); }, 60000);
        session.undo();
        const auto back = TestUtils::runUntil ([&] { return renderer.isUpToDate(); }, 300);
        check (waits && again && back && clipOf (*session.getState(), w.id).effects.rendered != nullptr,
               "clip effects renderer: a changed chain is rendered again, and an undo has its render back at once");

        // A plug-in that isn't there: told once, and the clip plays dry rather than waiting.
        juce::PluginDescription missing;
        missing.name = "Nowhere Verb";
        missing.pluginFormatName = "VST3";
        missing.fileOrIdentifier = "/Library/Audio/Plug-Ins/VST3/Nowhere Verb.vst3";
        auto broken = *session.getState();
        SessionEdits::setClipEffects (broken, w.id, 0.0, ClipEffects::withSlot ({}, missing));
        session.apply (std::move (broken), "Add Clip Effect");
        const auto settled = TestUtils::runUntil ([&] { return renderer.isUpToDate(); }, 30000);
        check (settled && failures.size() == 1 && failures[0].startsWith ("Voice: ") && clipOf (*session.getState(), w.id).effects.rendered == nullptr,
               "clip effects renderer: a chain that can't run is told once (" + failures.joinIntoString ("; ") + ") and the clip plays dry");
    }

    void testSessionFile (AudioEngine& engine)
    {
        const auto dir = freshFolder ("TorqwaveSoundDesignFile");
        const auto file = dir.getChildFile ("Voice.wav");
        const auto audio = tone (48000, 440.0);
        AudioFileIO::save (*audio, rate, file, 24, engine.formatManager);

        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        const auto a = SessionEdits::addClip (s, 0, file, audio, 0);
        const auto b = SessionEdits::addClip (s, 0, file, audio, 96000);
        const auto chain = ClipEffects::withBypass (ClipEffects::withSlot (ClipEffects::withSlot ({}, builtIn ("Reverb")), builtIn ("Compressor")), 1, true);
        SessionEdits::setClipEffects (s, a, 2.5, chain);

        const auto target = dir.getChildFile ("Song.tqsession");
        const auto saveError = SessionFile::save (s, target);
        const auto text = target.loadFileAsString();
        const auto loaded = SessionFile::load (target, engine.formatManager);
        const auto& tracks = loaded.state.tracks;
        const auto ok = saveError.isEmpty() && loaded.error.isEmpty() && tracks.size() == 1 && tracks[0].clips.size() == 2;
        const auto& la = tracks[0].clips[0];
        const auto& lb = tracks[0].clips[1];
        const auto slots = ok ? ClipEffects::slotsOf (la.effects.chain) : std::vector<ClipEffects::Slot>();
        check (ok && la.effects.semitones == 2.5 && la.effects.chain == clipOf (s, a).effects.chain && slots.size() == 2 && slots[0].plugin.name == "Reverb"
                   && ! slots[0].bypassed && slots[1].bypassed && la.effects.rendered == nullptr && ! lb.effects.isOn() && text.contains ("CLIPEFFECTS"),
               "clip effects: a clip's pitch shift and chain (bypass and all) round trip through the session file; a clip without them has none");

        // A session without clip effects is written as before, with no trace of them.
        auto none = s;
        SessionEdits::setClipEffects (none, a, 0.0, {});
        SessionFile::save (none, target);
        const auto plainText = target.loadFileAsString();
        const auto reloaded = SessionFile::load (target, engine.formatManager);
        check (! plainText.contains ("CLIPEFFECTS") && reloaded.error.isEmpty() && ! reloaded.state.tracks[0].clips[0].effects.isOn(),
               "clip effects: a session without them saves no CLIPEFFECTS, so older versions read it as they did");
        juce::ignoreUnused (b);
        dir.deleteRecursively();
    }

    //==============================================================================
    void testCombine (AudioEngine& engine)
    {
        // Two layers at 0 and 0.25 s on two tracks (the second shifted up five semitones, at -6 dB
        // and panned), a clip on a third track under them and one later on the first, not chosen.
        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        s.addTrack();
        s.addTrack();
        const auto a = SessionEdits::addClip (s, 0, juce::File ("/takes/a.wav"), tone (48000, 220.0), 0);
        const auto b = SessionEdits::addClip (s, 1, juce::File ("/takes/b.wav"), tone (48000, 330.0), 12000);
        const auto under = SessionEdits::addClip (s, 2, juce::File ("/takes/c.wav"), tone (96000, 55.0), 0);
        const auto later = SessionEdits::addClip (s, 0, juce::File ("/takes/d.wav"), tone (24000, 660.0), 100000);
        s.tracks[1].gainDb = -6.0f;
        s.tracks[1].pan = 0.3f;
        SessionEdits::setClipEffects (s, b, 5.0, {});
        juce::String errors;
        onWorker ([&] { errors = ClipEffects::renderAll (s, engine.pluginFormatManager); });

        const std::vector<juce::uint32> ids { a, b };
        const auto range = SoundDesign::combineRange (s, ids);
        const auto alone = SoundDesign::combineSource (s, ids);
        const auto combined = mixDown (alone, range);

        // What the stack plays: the whole session over its span, everything else muted.
        auto stack = s;
        clipOf (stack, under).muted = clipOf (stack, later).muted = true;
        const auto heard = mixDown (stack, range);
        check (errors.isEmpty() && range == juce::Range<juce::int64> (0, 60000) && alone.tracks.size() == 2 && combined.getNumSamples() == 60000
                   && combined.getMagnitude (0, combined.getNumSamples()) > 0.1f && same (combined, heard),
               "combine: the chosen clips render (" + juce::String (combined.getNumSamples()) + " frames) exactly as the stack plays them, "
               "with their pitch, tracks' volume and pan; other clips left out");

        // Landed in the session: the layers muted, the sound on a new track under them, playing the same.
        auto landed = s;
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("Layers.wav");
        const auto id = SoundDesign::landCombined (landed, ids, file, AudioEdits::fromBuffer (juce::AudioBuffer<float> (combined)), range.getStart(), "Layers");
        const auto where = landed.findClip (id);
        auto check2 = landed;
        clipOf (check2, under).muted = clipOf (check2, later).muted = true;
        const auto playsBack = worst (mixDown (check2, range), combined);
        check (id != 0 && where.has_value() && where->first == 2 && landed.tracks.size() == 4 && landed.tracks[2].name == "Layers"
                   && clipOf (landed, a).muted && clipOf (landed, b).muted && ! clipOf (landed, under).muted && clipOf (landed, id).position == 0
                   && clipOf (landed, id).name == "Layers" && playsBack < 1.0e-6f,
               "combine: the new sound lands on a track after the layers' last, which are muted; the session sounds the same (worst difference "
                   + juce::String (playsBack, 9) + ")");
    }

    void testAddToLibrary (AudioEngine& engine)
    {
        const auto dir = freshFolder ("TorqwaveSoundDesignLibrary");
        SoundLibrary::Index index (dir.getChildFile ("Index.sqlite"));
        const auto folder = dir.getChildFile ("Sounds");
        const auto& entry = Ucs::list()[5];
        const auto tags = SoundDesign::tagsFrom ("creaky, Wood ,, CREAKY , old");
        const auto sound = tone (24000, 880.0);

        const auto filed = SoundDesign::addToLibrary (index, *sound, rate, folder, "Door: Creak", entry.catId, tags, engine.formatManager);
        SoundLibrary::Query byTag;
        byTag.tag = "creaky";
        const auto found = index.find (byTag);
        SoundLibrary::Query byCategory;
        byCategory.catId = entry.catId;
        check (tags == juce::StringArray { "creaky", "Wood", "old" } && filed.error.isEmpty() && filed.file.existsAsFile() && filed.file.getParentDirectory() == folder
                   && index.getFolders().contains (folder) && found.size() == 1 && found[0].id == filed.id && found[0].catId == entry.catId
                   && found[0].tags.size() == 3 && found[0].frames == 24000 && index.count (byCategory) == 1,
               "add to library: the sound is written into the folder (now watched) and is in the library with its category ("
                   + entry.catId + ") and tags (" + (found.empty() ? juce::String() : found[0].tags.joinIntoString (", ")) + ")");

        // In the file too, for other apps.
        juce::String ixml;

        for (auto& chunk : SoundMetadata::chunksOf (filed.file))
            if (chunk.id == "iXML")
            {
                juce::FileInputStream in (filed.file);
                juce::MemoryBlock data;
                in.setPosition (chunk.start);
                in.readIntoMemoryBlock (data, (ssize_t) chunk.size);
                ixml = data.toString();
            }

        const auto back = AudioFileIO::load (filed.file, engine.formatManager);
        check (ixml.contains (entry.catId) && ixml.contains ("creaky") && ixml.contains ("Wood") && back.snapshot != nullptr
                   && back.snapshot->getNumFrames() == 24000 && worst (bufferOf (back.snapshot), bufferOf (sound)) < 1.0e-6f,
               "add to library: the file holds the category and tags in its iXML, and the sound at 24 bits");

        const auto second = SoundDesign::addToLibrary (index, *sound, rate, folder, "Door: Creak", {}, {}, engine.formatManager);
        const auto wrong = SoundDesign::addToLibrary (index, *sound, rate, folder, "Other", "NOTACATEGORY", {}, engine.formatManager);
        const auto unnamed = SoundDesign::addToLibrary (index, *sound, rate, folder, "  ", {}, {}, engine.formatManager);
        check (second.error.isEmpty() && second.file != filed.file && second.id != filed.id && index.count() == 2
                   && wrong.error.contains ("NOTACATEGORY") && wrong.file == juce::File() && unnamed.error.isNotEmpty(),
               "add to library: the same name again is a second file; a category not in the UCS list and no name are refused");
        dir.deleteRecursively();
    }

    //==============================================================================
    void testOpenFilesLink()
    {
        AudioEngine engine;
        OpenFiles files (engine);
        files.prepareForAnother ({});
        AudioFileIO::Loaded first;
        first.snapshot = tone (1000, 100.0);
        first.sampleRate = rate;
        engine.adoptFile (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("a.wav"), std::move (first));

        auto link = std::make_shared<SoundDesign::ClipEdit>();
        link->clipId = 7;
        link->clipName = "Kick";
        files.prepareForAnother ({});
        AudioFileIO::Loaded clip;
        clip.snapshot = tone (1000, 200.0);
        clip.sampleRate = rate;
        engine.adoptFile ({}, std::move (clip));
        files.setLink (files.getActiveIndex(), link);

        const auto linked = files.size() == 2 && files.nameAt (1) == "Kick (Clip)" && files.nameAt (0) == "a.wav" && files.indexOfClip (7) == 1
                            && files.indexOfClip (8) == -1 && files.linkAt (0) == nullptr && files.linkAt (1) == link;
        files.close (1);
        check (linked && files.indexOfClip (7) == -1 && files.size() == 1,
               "edit in place: the editor's file is linked to its clip, named after it, found by it, and the link goes with the file");
    }

    void testDoubleClick (AudioEngine& engine)
    {
        SessionState s;
        s.sampleRate = rate;
        s.addTrack();
        const auto id = SessionEdits::addClip (s, 0, juce::File ("/takes/a.wav"), tone (96000, 220.0), 48000);
        engine.session.reset (s, {});

        constexpr int timelineWidth = 960;
        MultitrackView view (engine);
        view.setSize (MultitrackView::headerWidth + timelineWidth + 12, MultitrackView::rulerHeight + 2 * MultitrackView::trackHeight + 12);
        view.zoomToRange ({ 0, 480000 });
        juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
        const auto x = [&] (juce::int64 frame)
        {
            const auto visible = view.getVisibleFrames();
            return (float) MultitrackView::headerWidth + (float) (((double) frame - visible.getStart()) / visible.getLength() * timelineWidth);
        };
        const auto doubleClick = [&] (juce::Point<float> at)
        {
            const auto now = juce::Time::getCurrentTime();
            view.mouseDoubleClick ({ juce::Desktop::getInstance().getMainMouseSource(), at, {}, juce::MouseInputSource::defaultPressure,
                                     juce::MouseInputSource::defaultOrientation, juce::MouseInputSource::defaultRotation,
                                     juce::MouseInputSource::defaultTiltX, juce::MouseInputSource::defaultTiltY,
                                     &view, &view, now, at, now, 2, false });
            juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
        };

        std::vector<juce::uint32> asked;
        view.onEditClip = [&asked] (juce::uint32 clipId) { asked.push_back (clipId); };
        const auto y = (float) MultitrackView::rulerHeight + 60.0f;
        doubleClick ({ x (96000), y });
        doubleClick ({ x (300000), y });
        check (asked == std::vector<juce::uint32> { id }, "edit in place: double-clicking a clip asks to edit it in place; an empty spot doesn't");
    }

    void testPanel (AudioEngine& engine)
    {
        auto w = withEffects();
        SessionEdits::setClipEffects (w.s, w.id, 0.0, {});
        engine.session.reset (w.s, {});
        ClipEffectsPanel panel (engine, w.id);
        panel.setSize (520, 360);
        const auto settle = [] { juce::MessageManager::getInstance()->runDispatchLoopUntil (40); };
        const auto effects = [&] { return clipOf (*engine.session.getState(), w.id).effects; };

        const auto pitched = panel.setSemitones (-2.0) && effects().semitones == -2.0 && engine.session.getUndoDescription() == "Clip Pitch";
        const auto added = panel.addEffect (builtIn ("Reverb")) && panel.addEffect (builtIn ("Compressor"));
        settle();
        const auto two = panel.describeRows() == juce::StringArray { "Reverb", "Compressor" };

        panel.rowButton (0, 1)->triggerClick();   // On, turned off
        settle();
        const auto off = ClipEffects::slotsOf (effects().chain)[0].bypassed && engine.session.getUndoDescription() == "Bypass Clip Effect"
                         && panel.describeRows()[0] == "Reverb (off)";
        panel.rowButton (1, 2)->triggerClick();   // Compressor up
        settle();
        const auto moved = panel.describeRows() == juce::StringArray { "Compressor", "Reverb (off)" };
        panel.rowButton (0, 4)->triggerClick();   // Compressor removed
        settle();
        const auto removed = panel.describeRows() == juce::StringArray { "Reverb (off)" } && engine.session.getUndoDescription() == "Remove Clip Effect";
        engine.session.undo();
        settle();
        check (pitched && added && two && off && moved && removed && panel.describeRows().size() == 2,
               "clip effects panel: pitch, effects added, one turned off, moved up and removed with its buttons, each an undo step");
        engine.session.reset ({}, {});
    }
}

void runSoundDesignTests (AudioEngine& engine)
{
    testEditInPlace();
    testFollowingEdit();
    testClipEffects (engine);
    testRenderer (engine);
    testSessionFile (engine);
    testCombine (engine);
    testAddToLibrary (engine);
    testOpenFilesLink();
    testDoubleClick (engine);
    testPanel (engine);
}
