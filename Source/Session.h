#pragma once

#include "AudioDocument.h"
#include "FadeShape.h"
#include "Metronome.h"
#include "TimeFormat.h"

/** A point on an envelope: the value at a frame (of the session, or for a clip's gain, of its audio). */
struct AutomationPoint
{
    juce::int64 frame = 0;
    float value = 0.0f;

    bool operator== (const AutomationPoint& o) const noexcept { return frame == o.frame && juce::exactlyEqual (value, o.value); }
};

/** How a value (a track's volume or pan, a clip's gain) changes over time: points in time
    order, joined by straight lines. Before the first point the value is the first point's,
    after the last the last's. An envelope with no points leaves the setting in charge. */
struct Envelope
{
    std::vector<AutomationPoint> points;

    bool isEmpty() const noexcept                 { return points.empty(); }
    float valueAt (juce::int64 frame) const;

    /** The values at frames start to start + n - 1; the same as valueAt at each, however the
        frames are split into blocks. */
    void fill (juce::int64 start, float* out, int n) const;

    /** Adds a point (after any others at the same frame, not before 0); returns its index. */
    int add (juce::int64 frame, float value);

    /** Moves a point, keeping it between its neighbours in time and not before 0. Returns its index. */
    int move (int index, juce::int64 frame, float value);

    void remove (int index);

    bool operator== (const Envelope& o) const noexcept { return points == o.points; }
};

//==============================================================================
/** A clip: a stretch of an audio file placed on a track. The audio is shared, never copied. */
struct SessionClip
{
    juce::uint32 id = 0;
    juce::String name;
    juce::File source;                     // the file the audio came from
    AudioDocument::Snapshot audio;
    juce::int64 position = 0;              // session frame where the clip starts
    juce::int64 offset = 0;                // first frame of the audio it plays
    juce::int64 length = 0;                // frames it plays
    float gainDb = 0.0f;                   // from minGainDb to maxGainDb
    juce::int64 fadeIn = 0, fadeOut = 0;   // frames
    FadeShape fadeInShape = FadeShape::sCurve, fadeOutShape = FadeShape::sCurve;
    bool locked = false;                   // the edits below leave it where it is, as it is
    bool muted = false;                    // stays in place but isn't heard (in playback, mixdowns, bounces and stems)
    int colour = -1;                       // a place in TrackColours::palette, or -1 for its track's colour
    juce::uint32 groupId = 0;              // clips sharing a non-zero id are selected (so moved and deleted) together

    /** Gain in dB on top of gainDb, from minGainDb to maxGainDb, its points at frames of the
        clip's audio, so the line stays with the sound when the clip is moved, trimmed or split.
        Empty: none. */
    Envelope gainEnvelope;
    juce::String note;                     // free text, shown along the clip's bottom edge

    static constexpr float minGainDb = -60.0f, maxGainDb = 24.0f;

    /** Other recordings of the same stretch (loop recording's passes), one of them in use. A
        take's audio is read `shift` frames on from where take 0's would be, so trims and splits
        apply to every take at once. With takes, source, audio and offset are the active take's. */
    struct Take
    {
        juce::File source;
        AudioDocument::Snapshot audio;
        juce::int64 shift = 0;
    };

    std::vector<Take> takes;
    int activeTake = 0;

    /** A comp: the clip plays a different take in each part. A segment plays its take from
        `from` up to the next segment's `from`; the first plays from the clip's start whatever
        its `from`. Frames are of take 0's audio (take k's frame is that plus its shift), so the
        comp stays with the sound when the clip is moved, trimmed, split or another take chosen.
        Sorted, no two neighbours alike. Empty: the active take throughout. */
    struct CompSegment
    {
        juce::int64 from = 0;
        int take = 0;

        bool operator== (const CompSegment& o) const noexcept { return from == o.from && take == o.take; }
    };

    std::vector<CompSegment> comp;
    juce::int64 compFade = -1;             // frames: the equal-power crossfade centred on each join (shorter
                                           // where a segment beside it is); -1 until the first comp sets 10 ms

    /** The take-0 frame the clip plays at session frame `frame` (its active take's audio frame
        less that take's shift). */
    juce::int64 takeZeroFrame (juce::int64 frame) const noexcept
    {
        return offset + (frame - position) - (takes.empty() ? 0 : takes[(size_t) activeTake].shift);
    }

    /** The take the comp plays at a take-0 frame: the active one without a comp. */
    int takeAt (juce::int64 takeZero) const noexcept
    {
        if (comp.empty())
            return activeTake;

        auto take = comp.front().take;

        for (size_t i = 1; i < comp.size() && comp[i].from <= takeZero; ++i)
            take = comp[i].take;

        return take;
    }

    juce::int64 getEnd() const noexcept    { return position + length; }

    /** Audio that follows the tempo (see TempoFollow.h). The clip's audio is then `original`
        stretched to the session's tempo map, from the beat where the original's first frame
        plays, so that its beats land on the session's: `audio`, `offset` and `length` are of that
        stretched audio, as for any clip, so playback and mixdowns read it as they read any other.
        It is stretched again, outside the undo steps, whenever the tempo map, the clip's tempo or
        its warp markers no longer match what it was stretched for (`renderedFor`). */
    struct WarpMarker
    {
        juce::int64 frame = 0;             // a frame of the original audio
        double beat = 0.0;                 // the beat it plays on, counted from where the original's first frame plays

        bool operator== (const WarpMarker& o) const noexcept { return frame == o.frame && juce::exactlyEqual (beat, o.beat); }
    };

    struct TempoFollow
    {
        double bpm = 0.0;                  // the audio's own tempo; 0: the clip plays at its own speed
        AudioDocument::Snapshot original;  // the audio as it is in its file (`source`)
        std::vector<WarpMarker> warps;     // sorted, frames and beats each increasing; the first frame is pinned to beat 0
        juce::String renderedFor;          // what `audio` was stretched for (TempoFollow::key); empty: not stretched yet

        bool follows() const noexcept      { return bpm > 0.0 && original != nullptr; }
    };

    TempoFollow tempoFollow;

    /** Clip effects, as Cubase's: effects on this clip alone, changing nothing in its files. The
        clip's audio (what it plays of its takes and comp) is shifted `semitones` (keeping its
        length), then goes through `chain`, a rack of effects (a TORQWAVE_RACK element, as
        AudioEngine::createRackXml() writes it, on one line), before its fades, gain line and gain.

        That is rendered in the background (ClipEffects::Renderer) into `rendered`, as long as the
        clip from its start, which playback, mixdowns and bounces read in place of the audio, so
        they hear the same samples at any block size and nothing is processed on the audio thread.
        A clip waiting for its render plays dry. `renderedFor` is the ClipEffects::key of what was
        rendered: a Session drops a render that no longer matches its clip (ClipEffects::dropStale),
        so a render of other audio is never heard. */
    struct Effects
    {
        double semitones = 0.0;            // -maxSemitones to +maxSemitones
        juce::String chain;                // empty: none
        AudioDocument::Snapshot rendered;
        juce::String renderedFor;

        static constexpr double maxSemitones = 12.0;   // as far as TimeStretch::shiftPitch goes

        bool isOn() const noexcept         { return ! juce::exactlyEqual (semitones, 0.0) || chain.isNotEmpty(); }
    };

    Effects effects;
};

namespace Comp
{
    /** Reads what a clip's comp plays from take-0 frame takeZero (see SessionClip::comp) into
        [0, count) of dest, as many channels as dest has, joins crossfaded; a take with no audio
        there is silent. `other` must have dest's channels and count frames: it holds the other
        side of a join. Allocates nothing. The clip must have a comp. */
    void read (const SessionClip&, juce::int64 takeZero, int count, juce::AudioBuffer<float>& dest, juce::AudioBuffer<float>& other);
}

/** Part of a track's signal sent to a bus, from before its volume (pre-fader) or after it. */
struct SessionSend
{
    juce::uint32 busId = 0;
    float gainDb = 0.0f;
    bool preFader = false;

    bool operator== (const SessionSend& o) const noexcept
    {
        return busId == o.busId && juce::exactlyEqual (gainDb, o.gainDb) && preFader == o.preFader;
    }
};

/** What an envelope automates: a track's volume or pan, or a parameter of one of its effects;
    or where it plays in a 5.1 master: its surround pan's x, y and width, and its LFE share. */
enum class AutomationParameter { volume, pan, effect, surroundX, surroundY, surroundWidth, lfe };

/** How a track's or bus's automation plays and is written, as in Cubase:
    - off: its envelopes are ignored, and its controls set its settings;
    - read: its envelopes play, and its automated controls follow them;
    - touch: as read, but a control moved while playing writes into its envelope until it is let
      go, then the envelope takes over again;
    - latch: as touch, but once moved a control goes on writing its last value until playback stops;
    - write: from play to stop its volume and pan are written as their controls stand, moved or not. */
enum class AutomationMode { off, read, touch, latch, write };

juce::String automationModeName (AutomationMode);
std::optional<AutomationMode> automationModeNamed (const juce::String&);

/** The surround pan's parameters, in SessionTrack::surroundEnvelopes' order. */
inline constexpr std::array<AutomationParameter, 4> surroundParameters { AutomationParameter::surroundX, AutomationParameter::surroundY,
                                                                         AutomationParameter::surroundWidth, AutomationParameter::lfe };

/** An envelope's target. For an effect parameter: the effect's slot (its TrackEffects slot id,
    which stays with the effect when the chain is reordered) and the parameter's ID. */
struct AutomationTarget
{
    AutomationParameter parameter = AutomationParameter::volume;
    juce::uint32 slotId = 0;
    juce::String parameterId;

    AutomationTarget (AutomationParameter p = AutomationParameter::volume) : parameter (p) {}   // implicit, so volume and pan convert
    static AutomationTarget effect (juce::uint32 slot, const juce::String& id)
    {
        AutomationTarget t (AutomationParameter::effect);
        t.slotId = slot;
        t.parameterId = id;
        return t;
    }

    bool operator== (const AutomationTarget& o) const noexcept
    {
        return parameter == o.parameter && slotId == o.slotId && parameterId == o.parameterId;
    }
};

/** The values an envelope's points can take: volume in dB (the track fader's range), pan from
    -1 (left) to +1 (right), an effect parameter from 0 to 1 (its normalised range), the surround
    x and y from -1 to +1, its width from 0 to 1, and the LFE from -60 dB (off there) to +10 dB. */
juce::Range<float> automationRange (AutomationParameter);

/** An envelope on a parameter of one of a track's effects. */
struct EffectEnvelope
{
    juce::uint32 slotId = 0;
    juce::String parameterId;
    Envelope envelope;                     // 0 to 1

    bool operator== (const EffectEnvelope& o) const noexcept
    {
        return slotId == o.slotId && parameterId == o.parameterId && envelope == o.envelope;
    }
};

/** A track: clips (overlapping clips mix), a level, a pan, mute, solo and sends to buses.
    Volume and pan can each follow an envelope instead of their setting, and so can its
    effects' parameters. An envelope for an effect that has left the chain is kept but unused.

    A bus is a track with no clips that plays what other tracks send it, through its own effects,
    volume and pan, into the mix or into another bus (its output). Buses have no sends. */
/** The device inputs a track records from: a first channel (absolute) and 1 or 2 channels, or
    the recording input chosen in the window (first < 0). */
struct TrackInput
{
    int first = -1, channels = 0;

    bool isDefault() const noexcept                       { return first < 0 || channels <= 0; }
    bool operator== (const TrackInput& o) const noexcept  { return isDefault() ? o.isDefault() : first == o.first && channels == o.channels; }
    bool operator!= (const TrackInput& o) const noexcept  { return ! operator== (o); }
};

/** A track's level dipping while another track (its key, a voice say) is heard: when the key's
    output is above the threshold, the track comes down by the amount, over the attack time,
    and goes back up over the release time once the key falls below it. */
struct Ducking
{
    juce::uint32 keyTrackId = 0;           // 0: no ducking
    float amountDb = -12.0f;               // -60 to -1
    float thresholdDb = -40.0f;            // -80 to 0 dBFS
    float attackMs = 10.0f;                // 1 to 500
    float releaseMs = 300.0f;              // 10 to 5000

    bool isOn() const noexcept             { return keyTrackId != 0; }
    bool isValid() const noexcept
    {
        return amountDb >= -60.0f && amountDb <= -1.0f && thresholdDb >= -80.0f && thresholdDb <= 0.0f
               && attackMs >= 1.0f && attackMs <= 500.0f && releaseMs >= 10.0f && releaseMs <= 5000.0f;
    }
};

/** A plug-in on a track or bus listening to another track: the key track's output (after its
    volume, as ducking hears it) goes into the plug-in's sidechain input. */
struct Sidechain
{
    juce::uint32 slotId = 0;               // the effect slot (TrackEffects) whose sidechain it feeds
    juce::uint32 keyTrackId = 0;           // a track, not a bus or folder
};

/** Where a track or bus plays in a 5.1 master (SessionState::surround): a point in the room,
    x from -1 (left) to +1 (right) and y from -1 (behind) to +1 (in front), seen from the middle,
    and how much goes to the LFE as well. Its direction picks the speakers (L R C Ls Rs); its
    distance from the middle, up to 1 at the edge, how much stays in them rather than spreading
    over all five. Ignored in a stereo master. */
struct SurroundPan
{
    float x = 0.0f, y = 1.0f;              // in front, at the edge
    float width = 1.0f;                    // 1: the left and right channels 30 degrees either side of the point (to L and R
                                           // from the front); 0: both at the point, as one mono source
    float lfeDb = lfeOff;                  // the LFE's share, in dB; lfeOff for none

    static constexpr float lfeOff = -150.0f, maxLfeDb = 10.0f;

    bool isDefault() const noexcept        { return *this == SurroundPan {}; }
    bool operator== (const SurroundPan& o) const noexcept
    {
        return juce::exactlyEqual (x, o.x) && juce::exactlyEqual (y, o.y) && juce::exactlyEqual (width, o.width)
            && juce::exactlyEqual (lfeDb, o.lfeDb);
    }
};

/** A note in a note clip: its MIDI pitch (60 is middle C), where it starts and how long it is
    held, in frames from the clip's start, and how hard it is played, from 0 to 1. */
struct ClipNote
{
    int pitch = 60;
    juce::int64 start = 0, length = 0;
    float velocity = 0.8f;
    float probability = 1.0f;   // the chance it plays on each pass, 0 to 1 (see NoteClip::plays)

    bool operator== (const ClipNote& o) const noexcept
    {
        return pitch == o.pitch && start == o.start && length == o.length && juce::exactlyEqual (velocity, o.velocity)
            && juce::exactlyEqual (probability, o.probability);
    }
};

/** How Quantize Notes and the drum grid place notes: on a grid of the beat, swung or straight. */
struct NoteQuantize
{
    int division = 4;              // grid lines in a beat: 4 is 1/16 notes in 4/4
    double swing = 0.5;            // where each pair's second line falls in it: 0.5 straight, up to 0.75
    double strength = 1.0;         // how far a note moves to its line, 0 to 1
    bool lengths = false;          // the notes' ends too

    static constexpr double maxSwing = 0.75;

    /** Swing applies on a grid of 2 or 4 lines a beat (1/8 or 1/16 notes in 4/4); others are straight. */
    bool swings() const noexcept   { return swing > 0.5 && (division == 2 || division == 4); }

    /** Grid line k's beat position from beat 0: the second line of each pair moved later by the swing. */
    double lineBeat (juce::int64 k) const noexcept;

    /** The line nearest a beat position. */
    double nearestLine (double beat) const noexcept;
};

/** A controller's course over a note clip's timeline: a CC (0 to 127) or the pitch bend
    (pitchBend), as an envelope of values from 0 to 1 at frames of the timeline (a CC's 0 to 127;
    the bend from all the way down to all the way up, 0.5 at rest), repeating with the pattern.
    Sent to the instrument with the notes: at the clip's start and each pass's, and at each
    automation step (TrackEffects::automationStep frames of the session) where the value it rounds
    to changed, before the notes at that frame. With no points the lane sends nothing. */
/** Frames between the values a lane sends (TrackEffects::automationStep). */
inline constexpr int automationStepFrames = 64;

struct ControllerLane
{
    static constexpr int pitchBend = 128;
    int controller = 1;                    // the mod wheel, or any CC 0 to 127, or pitchBend
    Envelope envelope;

    bool operator== (const ControllerLane& o) const noexcept { return controller == o.controller && envelope == o.envelope; }

    /** The lane's name: a CC's usual meaning where it has one. */
    static juce::String nameOf (int controller);
};

/** Notes placed on a track, played into its instrument (the first slot of its chain, when it
    is one; a track without an instrument plays nothing from them): a clip at a session position,
    length frames long, its notes at frames of their own timeline, which starts `offset` frames
    before the clip does (so trimming or splitting the clip leaves the notes where they were in
    the session). With a loop length the notes repeat every that many frames from that origin up
    to the clip's end; a note the clip's end cuts turns off there. Notes starting before the clip,
    at or past its end, or at or past the loop length, play nothing. */
struct NoteClip
{
    juce::uint32 id = 0;
    juce::String name;
    juce::int64 position = 0;              // session frame where the clip starts
    juce::int64 offset = 0;                // frames of the notes' timeline before the clip's start
    juce::int64 length = 0;                // frames it plays
    juce::int64 loopLength = 0;            // frames between repeats of the notes, or 0 to play them once
    std::vector<ClipNote> notes;
    std::vector<ControllerLane> lanes;     // at most one a controller
    bool muted = false;                    // stays in place but plays nothing
    int colour = -1;                       // a place in TrackColours::palette, or -1 for its track's colour
    int channel = 1;                       // the MIDI channel its notes and lanes go out on, 1 to 16

    juce::int64 getEnd() const noexcept    { return position + length; }

    /** The lane for a controller, or null. */
    const ControllerLane* findLane (int controller) const noexcept;

    /** A lane's value at a frame of the session (its envelope at the timeline frame, repeating
        with the pattern), or nothing outside the clip or without points. */
    std::optional<float> laneValueAt (const ControllerLane&, juce::int64 frame) const noexcept;

    /** The MIDI message for a lane's value: a controller change or a pitch wheel move, on a channel (1 to 16). */
    static juce::MidiMessage laneMessage (int controller, float value, int channel = 1);

    /** The session frame the notes' timeline starts at. */
    juce::int64 getOrigin() const noexcept { return position - offset; }

    /** Frames between passes of the notes: the loop length, or without one the whole timeline. */
    juce::int64 getPeriod() const noexcept { return loopLength > 0 ? loopLength : offset + length; }

    /** The MIDI the clip plays in session frames [from, from + count): each note's note-on at its
        frame and note-off where it ends (or at the clip's end), as frames from 0, on its channel.
        At one frame note-offs come before note-ons, so a note taking over from another doesn't
        lose its voice. The same whatever the blocks: a note's events fall in whichever block has
        their frame. */
    void addMidi (juce::int64 from, int count, juce::MidiBuffer&) const;

    /** Whether note `index` plays on pass `pass` (0: the first pass from the origin): always at
        a probability of 1, otherwise as a fixed draw from the clip's id, the pass and the note's
        place in the list, so playback, a mixdown and every block size agree. */
    bool plays (size_t index, juce::int64 pass) const noexcept;

    /** The frames the notes span on their timeline, from the first start to the last end, within offset + length. */
    juce::Range<juce::int64> notesRange() const noexcept;

    bool operator== (const NoteClip& o) const noexcept
    {
        return id == o.id && name == o.name && position == o.position && offset == o.offset && length == o.length && loopLength == o.loopLength
            && notes == o.notes && lanes == o.lanes && muted == o.muted && colour == o.colour && channel == o.channel;
    }
};

/** MIDI learn: a controller's CC on a channel moving a control in the session. Volume and pan
    take the CC's value across their range (the volume on the mixer fader's taper); mute, solo and
    the transport act when it goes to 64 or over (a button pressed). */
struct MidiBinding
{
    enum class Kind { volume, pan, mute, solo, play, stop, record };

    struct Target
    {
        Kind kind = Kind::volume;
        juce::uint32 trackId = 0;          // for a track's control; 0 for the transport

        bool operator== (const Target& o) const noexcept { return kind == o.kind && trackId == o.trackId; }
        bool isTrack() const noexcept    { return kind == Kind::volume || kind == Kind::pan || kind == Kind::mute || kind == Kind::solo; }
    };

    int channel = 1;                       // 1 to 16
    int controller = 0;                    // 0 to 127
    Target target;

    bool operator== (const MidiBinding& o) const noexcept { return channel == o.channel && controller == o.controller && target == o.target; }

    static juce::String kindName (Kind);
    static std::optional<Kind> kindNamed (const juce::String&);
};

/** A drum sound: a row of the piano roll's drum grid, the note it plays and its name. */
struct DrumSound
{
    int pitch = 36;
    juce::String name;

    bool operator== (const DrumSound& o) const noexcept { return pitch == o.pitch && name == o.name; }

    /** General MIDI's percussion sounds, notes 35 to 81, the kick first. */
    static const std::vector<DrumSound>& generalMidi();

    /** The name of a note in a map, or its pitch name (C1, D#2...) when the map doesn't have it. */
    static juce::String nameIn (const std::vector<DrumSound>& map, int pitch);
};

struct SessionTrack
{
    juce::uint32 id = 0;
    juce::String name;
    bool isBus = false;

    /** The session's master track (a bus too): the whole mix plays through its effects, then its
        volume. It is always the last track, takes no sends or outputs, sits in no folder or VCA,
        and has no pan or solo. */
    bool isMaster = false;

    /** A bus that tracks and buses can send or play into: any bus but the master. */
    bool takesBusInput() const noexcept           { return isBus && ! isMaster; }

    float gainDb = 0.0f;
    float pan = 0.0f;                      // -1 (left) to +1 (right)
    bool mute = false, solo = false;
    bool soloSafe = false;                 // keeps playing when other tracks are soloed (a reverb return, say)
    bool invertPolarity = false;           // its output turned upside down, after its effects and before its volume
    int group = 0;                         // 0 for none, or 1 to maxGroups: volume, mute and solo changes follow through it
    int colour = -1;                       // a place in TrackColours::palette, or -1 for one by its place in the list
    bool hidden = false;                   // left out of the lanes (it still plays and shows in the mixer)

    /** A folder gathers tracks and buses: no audio of its own, but its mute, solo and volume
        (gainDb, added to each one's own) act on them all, and collapsed it leaves them out of
        the lanes (they still play). */
    bool isFolder = false;
    bool collapsed = false;                // a folder's members out of the lanes
    juce::uint32 folderId = 0;             // the folder this track or bus is in, or 0

    /** A VCA fader: no audio, clips or effects of its own. The tracks and buses that join it
        (vcaId) have its volume, or its volume envelope, added to their own in dB as they are
        mixed, without being summed together; its mute and solo act on them as if their own. */
    bool isVca = false;
    juce::uint32 vcaId = 0;                // the VCA fader this track or bus has joined, or 0

    /** A folder or a VCA fader: a track with no audio of its own. */
    bool isControl() const noexcept               { return isFolder || isVca; }

    AutomationMode automationMode = AutomationMode::read;

    /** False in Off: its envelopes don't play. */
    bool readsAutomation() const noexcept         { return automationMode != AutomationMode::off; }

    /** Touch, Latch or Write: moving a control while playing writes automation. */
    bool writesAutomation() const noexcept        { return automationMode >= AutomationMode::touch; }
    juce::String note;                     // free text, shown in the header's tooltip

    /** How the piano roll shows its note clips: as a drum grid (one row a sound), and the sounds,
        top to bottom. An empty map is General MIDI's. */
    bool drumGrid = false;
    std::vector<DrumSound> drumMap;

    /** An external instrument (tracks only): its note clips, controller lanes and live input go
        out of a MIDI output port (its device identifier, and the name it had), on their own
        channels or all on one (1 to 16; 0 keeps theirs), sent offsetMs earlier (negative) or
        later than the audio, to line up with what the synth sends back. */
    juce::String midiOutPort, midiOutName;
    int midiOutChannel = 0;
    double midiOutOffsetMs = 0.0;
    static constexpr double maxMidiOutOffsetMs = 500.0;

    bool playsMidiOut() const noexcept            { return midiOutPort.isNotEmpty() && ! isBus && ! isControl(); }

    /** An instrument track playing the MIDI another track's plug-ins put out (an arpeggiator,
        a chord or MIDI effect plug-in), as well as its own notes; or 0. */
    juce::uint32 midiFromTrackId = 0;

    /** The drum map in use: its own, or General MIDI's. */
    const std::vector<DrumSound>& drumSounds() const noexcept { return drumMap.empty() ? DrumSound::generalMidi() : drumMap; }

    TrackInput input;                      // what it records from (tracks only)

    /** Where a track's or bus's output goes: 0 for the mix, or a bus it plays into in place of
        the mix, after its volume. A track's sends are as they were. */
    juce::uint32 outputBusId = 0;

    /** Or a pair of the audio interface's outputs it plays out of in place of the mix (past the
        rack): the first channel of the pair, from 0, or -1 for none. Used only with outputBusId 0. */
    int deviceOutput = -1;
    static constexpr int maxDeviceOutputs = 32;   // pairs from 1/2 to 31/32

    /** Where it plays when its output goes to a 5.1 master. */
    SurroundPan surroundPan;

    /** A 5.1 track (tracks only): its clips play six channels, L R C LFE Ls Rs (a 5.1 clip as it
        is, a mono one to C, a stereo one to L and R, others their first six channels in order),
        through its polarity, volume and ducking straight to a 5.1 master's speakers, with no pan
        or surround pan. Its effects run on its channels in pairs (TrackEffects). What it sends, plays into a bus
        or out of an interface pair, ducks or keys, and plays into a stereo master, is folded down
        to stereo (Surround::foldDown). */
    bool fiveOne = false;

    /** A 7.1 track (tracks only), as a 5.1 track but eight channels, L R C LFE Lrs Rrs Lss Rss (a
        7.1 clip as it is, a 5.1 one with its surrounds in the side pair Lss Rss as in a 7.1 master, a
        mono one to C, a stereo one to L and R), its effects in four pairs. Into a 7.1
        master it plays straight to the speakers; into a 5.1 master its rear and side surrounds
        both go to Ls and Rs at -3 dB each. Never with fiveOne. */
    bool sevenOne = false;

    /** The channels a 5.1 or 7.1 track plays (6 or 8), 0 for any other. */
    int wideChannels() const noexcept       { return sevenOne ? 8 : fiveOne ? 6 : 0; }
    bool isWide() const noexcept            { return fiveOne || sevenOne; }

    /** Ducking under another track (tracks only). It acts with the volume: on the track's
        output and post-fader sends, not its pre-fader sends. */
    Ducking ducking;

    /** The effect slots fed a key track's output on their sidechain input, at most one key each. */
    std::vector<Sidechain> sidechains;

    /** The key feeding this slot's sidechain, or 0. */
    juce::uint32 sidechainKey (juce::uint32 slotId) const;

    /** A frozen track plays this (its clips, pan and effects rendered, from session frame 0)
        instead of its clips and effects; its volume, sends, mute and solo still apply. Its clips
        can't be changed while it is frozen. */
    AudioDocument::Snapshot frozenAudio;
    juce::File frozenFile;

    bool isFrozen() const noexcept                { return frozenAudio != nullptr; }

    /** An instrument track holds note clips (with their controller lanes), played by the
        instrument first in its chain; an audio track holds audio clips. Neither holds the
        other's: an edit that would put a clip on the wrong kind of track puts it on a new track
        of the right kind instead, or leaves it where it is. Never a bus or a folder. */
    bool isInstrument = false;

    /** The musical timebase: the track's clips keep their place in bars and beats when the tempo
        changes (each moved to the beat it started on; a note clip's notes, lanes, loop and
        length scaled with its bars; an audio clip's audio as long as it was, a locked one left),
        rather than their place in time. New instrument tracks start musical, audio tracks in
        time; never a bus or a folder. */
    bool musical = false;

    /** False for a bus, a folder or a frozen track: clips meant for it go onto a new track instead. */
    bool takesClips() const noexcept              { return ! isBus && ! isControl() && ! isFrozen(); }

    /** Which clips the track takes: audio clips on an audio track, note clips on an instrument track. */
    bool takesAudioClips() const noexcept         { return takesClips() && ! isInstrument; }
    bool takesNoteClips() const noexcept          { return takesClips() && isInstrument; }

    /** True when the track has anything to play: audio clips or note clips. */
    bool hasClips() const noexcept                { return ! clips.empty() || ! noteClips.empty(); }

    /** The end of the last audio or note clip (0 with none): how far a bounce, freeze or stem of the track reaches. */
    juce::int64 clipsEnd() const noexcept;

    static constexpr int maxGroups = 8;
    std::vector<SessionClip> clips;

    /** Notes for the track's instrument (tracks only; see NoteClip): trimmed, split, moved,
        copied with time and cleared with it as clips are; they go with the track. */
    std::vector<NoteClip> noteClips;
    std::vector<SessionSend> sends;
    Envelope volumeEnvelope, panEnvelope;  // dB, and -1 to +1

    /** The surround pan's envelopes, in surroundParameters' order: x, y, width, LFE (dB, off at -60). */
    std::array<Envelope, 4> surroundEnvelopes;

    /** True when any part of its surround pan follows an envelope. */
    bool hasSurroundAutomation() const noexcept
    {
        return std::any_of (surroundEnvelopes.begin(), surroundEnvelopes.end(), [] (auto& e) { return ! e.isEmpty(); });
    }

    /** Its surround pan at a session frame: each part from its envelope, or the setting. */
    SurroundPan surroundPanAt (juce::int64 frame) const;
    std::vector<EffectEnvelope> effectEnvelopes;   // only ones with points

    const SessionSend* findSend (juce::uint32 busId) const;

    /** The target's envelope; empty for an effect parameter with none. */
    const Envelope& envelope (const AutomationTarget&) const;

    /** True when the target plays its envelope: it has one, and the mode isn't Off. */
    bool follows (const AutomationTarget& t) const  { return readsAutomation() && ! envelope (t).isEmpty(); }

    /** The volume (dB) or pan at a session frame: the envelope's value, or the setting when
        there is no envelope. An effect parameter with no envelope has no setting here (its
        effect holds it) and gives 0. */
    float valueAt (const AutomationTarget&, juce::int64 frame) const;

    /** True when anything on the track is automated. */
    bool isAutomated() const noexcept
    {
        return ! volumeEnvelope.isEmpty() || ! panEnvelope.isEmpty() || ! effectEnvelopes.empty() || hasSurroundAutomation();
    }
};

/** A section of the arrangement (Intro, Verse, Chorus...): a named, coloured span of session
    time on the Arranger strip, holding everything on every track in it. */
struct ArrangerSection
{
    juce::uint32 id = 0;
    juce::String name;
    juce::int64 start = 0, length = 0;     // frames
    int colour = -1;                       // a place in TrackColours::palette, or -1 for the strip's own

    juce::int64 getEnd() const noexcept    { return start + length; }

    bool operator== (const ArrangerSection& o) const noexcept
    {
        return id == o.id && name == o.name && start == o.start && length == o.length && colour == o.colour;
    }
};

/** A mixer scene (a snapshot of the mix, as Cubase's mix snapshots): every track's, bus's,
    folder's and VCA fader's volume, pan, surround pan, mute, solo and polarity, and each track's
    sends (bus, level, before or after the fader), as they were when it was stored. Recalling it
    sets them back as one undo step: tracks added since keep their own mix, and sends to buses
    deleted since are left out. It holds no effects (their settings live outside the undo steps; a
    channel-strip preset carries them), automation, routing, groups or automation modes, and an
    envelope still takes over from a recalled volume or pan while it has points. */
struct MixScene
{
    struct Strip
    {
        juce::uint32 trackId = 0;
        float gainDb = 0.0f, pan = 0.0f;
        SurroundPan surroundPan;
        bool mute = false, solo = false, invertPolarity = false;
        std::vector<SessionSend> sends;

        bool operator== (const Strip& o) const noexcept
        {
            return trackId == o.trackId && juce::exactlyEqual (gainDb, o.gainDb) && juce::exactlyEqual (pan, o.pan) && surroundPan == o.surroundPan
                && mute == o.mute && solo == o.solo && invertPolarity == o.invertPolarity && sends == o.sends;
        }
    };

    juce::uint32 id = 0;
    juce::String name;
    std::vector<Strip> strips;             // in the session's order when stored

    /** The strip of the track with this id, or null. */
    const Strip* find (juce::uint32 trackId) const;

    /** A track's mix as a scene holds it. */
    static Strip stripOf (const SessionTrack&);
};

/** One state of a session. Copies share the clips' audio. */
struct SessionState
{
    double sampleRate = 48000.0;
    double tempo = 120.0;                  // beats a minute, for the metronome, from the start
    int beatsPerBar = 4;
    std::vector<Metronome::TempoChange> tempoChanges;   // at later bars, sorted by bar, one a bar
    juce::String notes;                                 // free text kept with the session

    /** The session's key, as NoteTools::parseKey reads it and Scale::name writes it ("A minor"),
        or empty for none (sessions from before it). The Sequence Library's Fits the Session
        filter shows the sequences in it. Set with SessionEdits::setKey. */
    juce::String musicalKey;

    /** The video the session goes with, shown in the Video panel, and where in the session (in
        seconds, 0 or later) its first frame plays. */
    juce::File video;
    double videoStart = 0.0;

    /** The session's timecode, as its picture's: a frame rate and the timecode at time 0. With
        one, the session's clock, ruler, markers and lists show it, whatever the app's time format;
        without (rate not a SMPTE kind, as in sessions from before it), they show the app's. */
    TimeFormat::Timecode timecode;
    std::vector<SessionTrack> tracks;
    Markers markers;                       // points and regions on the session's time, kept sorted
    juce::uint32 nextId = 1;

    /** The arrangement's sections, sorted by start, none overlapping (ids from nextId). Time
        edits move and cut them as they do regions. */
    std::vector<ArrangerSection> sections;

    /** The arrangement chain: sections by id, in the order to play them (one may come more than
        once). With playArrangement, playback goes from the end of each to the start of the
        next, and past the last plays on. Flatten Arrangement makes it the session's own time. */
    std::vector<juce::uint32> arrangement;
    bool playArrangement = false;

    /** MIDI-learned controllers (MidiLearn): one binding a controller, one a target. */
    std::vector<MidiBinding> midiBindings;

    /** Mixer scenes, in the order they were stored (ids from nextId). */
    std::vector<MixScene> scenes;
    const MixScene* findScene (juce::uint32 id) const;

    const ArrangerSection* findSection (juce::uint32 id) const;

    /** The master is 5.1 (L R C LFE Ls Rs) rather than stereo: what goes to the mix is placed by
        each track's and bus's surroundPan. */
    bool surround = false;

    /** With surround, the master is 7.1 (L R C LFE Lrs Rrs Lss Rss, as Surround names them) rather
        than 5.1. */
    bool sevenOne = false;

    /** Overlapping clips on a track layer, rather than mix: a clip later in its track's list (on
        top) hides the part of the ones under it it covers, except over its own fades, so
        crossfades still sound both. */
    bool layerClips = false;

    /** The loop: a range of session time that Loop Playback goes round (as a cycle between
        locators), whatever is selected; empty for none. Kept with the session, but set outside
        the undo steps (Session::setLoop). */
    juce::Range<juce::int64> loop;

    /** The ruler counts bars and beats of the tempo (and snaps to its beats). Kept with the
        session, outside the undo steps (Session::setBarsAndBeats). */
    bool barsAndBeats = false;

    /** The master's channels: 2, 6 for 5.1 or 8 for 7.1. */
    int masterChannels() const noexcept    { return ! surround ? 2 : sevenOne ? 8 : 6; }

    /** The end of the last clip. */
    juce::int64 getLength() const;

    /** The beats of the tempo and its changes. It refers to tempoChanges, so it lasts only as
        long as this state, unchanged. */
    Metronome::TempoMap tempoMap() const noexcept { return { tempo, beatsPerBar, tempoChanges, sampleRate }; }
    bool anySolo() const;

    /** Muted or soloed itself or through its folder. */
    bool isMuted (const SessionTrack&) const;
    bool isSoloed (const SessionTrack&) const;
    bool isSoloSafe (const SessionTrack&) const;

    /** Its folder's volume as a gain, by which its output and post-fader sends are scaled; 1
        out of any folder. */
    float folderGain (const SessionTrack&) const;

    /** The VCA fader a track or bus has joined, or nullptr. */
    const SessionTrack* vcaOf (const SessionTrack&) const;

    /** In the lanes: not hidden, nor in a collapsed folder. */
    bool isShown (const SessionTrack&) const;

    /** Adds an empty track named "Track n" and returns it. */
    SessionTrack& addTrack();

    /** Adds an empty instrument track named "Track n" and returns it. */
    SessionTrack& addInstrumentTrack();

    /** Adds a bus named "Bus n" and returns it. */
    SessionTrack& addBus();

    /** Adds an empty, open folder named "Folder n" and returns it. */
    SessionTrack& addFolder();
    SessionTrack& addVca();

    /** Adds the master track ("Master") when there is none, and returns it. */
    SessionTrack& addMaster();

    /** The master track, or null for a session without one (one from before it, until opened). */
    const SessionTrack* findMaster() const;

    /** Keeps the master as it must be: one, the last track, out of folders and VCAs, without
        sends, an output, pan or solo. Session::apply does it to every step. */
    void tidyMaster();

    /** The track (or bus) with this id, or null. */
    const SessionTrack* findTrack (juce::uint32 trackId) const;

    /** The bus a track's or bus's output goes to, or null for the mix. */
    const SessionTrack* outputBusOf (const SessionTrack&) const;

    /** True when what a track or bus plays reaches this bus: through a send or its output,
        then through the outputs of the buses on the way. */
    bool feeds (const SessionTrack& from, juce::uint32 busId) const;

    /** True when this track listens to the other, through any number of keys (ducking or a
        sidechain) in turn: the other must be mixed first. */
    bool listensTo (const SessionTrack& track, juce::uint32 otherId) const;

    /** The track and index of the clip with this id, or nothing. */
    std::optional<std::pair<int, int>> findClip (juce::uint32 clipId) const;

    /** The track and index of the note clip with this id, or nothing. */
    std::optional<std::pair<int, int>> findNoteClip (juce::uint32 clipId) const;
};

//==============================================================================
/** A stretch of a session's time copied from every track, for pasting as inserted time. Frames
    count from the stretch's start. */
struct SessionTime
{
    struct Track
    {
        std::vector<SessionClip> clips;            // cut to the stretch, unlocked
        std::vector<NoteClip> noteClips;           // cut to the stretch
        Envelope volume, pan;                      // empty where the track had none
        std::array<Envelope, 4> surround;          // likewise, in surroundParameters' order
        std::vector<EffectEnvelope> effects;
    };

    juce::int64 length = 0;
    double sampleRate = 0.0;
    std::vector<Track> tracks;                     // in the session's order, buses included
    Markers markers;
    std::vector<ArrangerSection> sections;         // the parts of sections in it, from 0

    /** When the time was whole bars, from a bar's start: how many, and their tempo and time
        signature, the first (bar 0) what played at its start, the rest the changes in it, from
        0. Pasted at a bar's start, they come too. 0 and empty otherwise. */
    juce::int64 bars = 0;
    std::vector<Metronome::TempoChange> tempo;
};

//==============================================================================
/** Edits on a session state. Each returns false (and leaves the state alone) when it cannot apply. */
namespace SessionEdits
{
    /** Adds a clip of the whole audio at position on a track (a new one when track is past the
        last or a bus; in a 5.1 master, a new track for six-channel audio is a 5.1 track). Returns
        the clip's id. */
    juce::uint32 addClip (SessionState&, int track, const juce::File& source, AudioDocument::Snapshot audio, juce::int64 position);

    /** Makes a track an instrument track or an audio track. Refused for a bus, a folder, a frozen
        or a 5.1 or 7.1 track, one already of that kind, or one holding clips of the kind it would
        no longer take. */
    bool setTrackKind (SessionState&, juce::uint32 trackId, bool instrument);

    /** Moves the clips on the wrong kind of track (audio clips on an instrument track, note clips
        on an audio track) onto a new track of the right kind after it, named like it. Returns how
        many tracks were added. For sessions saved before tracks had kinds. */
    int separateClipKinds (SessionState&);

    /** Gives a track the musical timebase or the time one (see SessionTrack::musical); false for
        a bus or a folder, when it has that already, or with no such track. */
    bool setTrackMusical (SessionState&, juce::uint32 trackId, bool musical);

    /** Moves a clip to a track (a new one when past the last) and a position (not before 0).
        Onto a bus, it only moves in time. */
    bool moveClip (SessionState&, juce::uint32 clipId, int toTrack, juce::int64 position);

    /** Automation follows the clips (after moveClip or moveNoteClip in `s`, from `before`): for each
        of these clips (audio or note) that moved in time and stayed on its track, the points of that
        track's envelopes (volume, pan, surround pan, effect parameters) under where it was move
        with it, the line across it kept as it was, replacing what was under where it went. An
        envelope with no point under the clip is left alone, and so are clips moved to another
        track. True when an envelope changed. */
    bool carryAutomation (SessionState& s, const SessionState& before, const std::vector<juce::uint32>& clipIds);

    /** Sets where a clip starts and ends in the session, showing more or less of its audio, which
        stays where it is in time. Limited to the audio there is; at least one frame long. */
    bool trimClip (SessionState&, juce::uint32 clipId, juce::int64 start, juce::int64 end);

    /** Splits a clip in two at a session frame strictly inside it. The first part keeps the fade
        in, the second the fade out. */
    bool splitClip (SessionState&, juce::uint32 clipId, juce::int64 frame);

    /** An audio clip follows the tempo at its own tempo `bpm` (20 to 400), or with 0 plays at its own
        speed again (see TempoFollow.h). It keeps where it starts and the part of its audio it plays,
        so its length changes with the stretch; its track becomes musical. False for a clip with
        takes, a locked one, a tempo out of range, or no change. */
    bool setClipTempo (SessionState&, juce::uint32 clipId, double bpm);

    /** A following clip's warp markers (frames of its original pinned to beats from where its first
        frame plays), sorted by frame; frames and beats must each increase, from after the first
        frame. Keeps where it starts and the part it plays. False for a clip that doesn't follow, a
        locked one, markers out of order, or no change. */
    bool setWarpMarkers (SessionState&, juce::uint32 clipId, std::vector<SessionClip::WarpMarker>);

    /** Splits a clip at each of these session frames inside it (Slice at Transients); how many it made. */
    int sliceClip (SessionState&, juce::uint32 clipId, std::vector<juce::int64> frames);

    /** Sets a clip's gain, limited to SessionClip::minGainDb to maxGainDb. */
    bool setClipGain (SessionState&, juce::uint32 clipId, float gainDb);

    /** Sets a clip's gain so that the loudest sample of the audio it plays reaches peakDb (within
        the gain's limits). False, changing nothing, when that audio is silent. */
    bool normalizeClip (SessionState&, juce::uint32 clipId, float peakDb);

    /** Sets a clip's fade lengths in frames. The fade in is limited to the clip's length and the
        fade out to what the fade in leaves. */
    bool setClipFades (SessionState&, juce::uint32 clipId, juce::int64 fadeIn, juce::int64 fadeOut);

    /** Sets the shape of a clip's fade in (or out). False for a locked clip or when it is so. */
    bool setClipFadeShape (SessionState&, juce::uint32 clipId, bool fadeOut, FadeShape);

    /** Shrinks each region to the span of the clips overlapping it (muted ones too), on any track,
        within the region; regions overlapping no clip are left alone. Returns how many changed. */
    int fitRegionsToClips (SessionState&);

    /** Adds a region over each stretch of the session where clips run without a gap (clips that
        overlap or touch, on any track, join into one), named as new regions are; stretches that
        already have a region exactly over them are skipped. Returns how many were added. */
    int regionsOverClips (SessionState&);

    bool removeClip (SessionState&, juce::uint32 clipId);

    /** Locks or unlocks a clip. Moving, trimming, splitting, fading, changing the gain, take or
        audio of a locked clip and removing it are refused (false); other clips placed or recorded
        over it, and crossfades with it, leave it as it is, so they overlap it. Removing its track
        still removes it. Copies of a locked clip start unlocked. */
    bool setClipLocked (SessionState&, juce::uint32 clipId, bool locked);

    /** The session frame nearest to `frame`, at most `within` frames away and inside the clip's
        audio, where its channels' sum crosses zero: the sum changes sign between the frame before
        and it, or is 0 there. `frame` itself when there is none. */
    juce::int64 zeroCrossingNear (const SessionClip&, juce::int64 frame, juce::int64 within);

    /** Sets a clip's gain envelope (see SessionClip::gainEnvelope), its values limited to
        SessionClip::minGainDb to maxGainDb. False for a locked clip, or one on a frozen track. */
    bool setClipGainEnvelope (SessionState&, juce::uint32 clipId, Envelope);

    /** Groups clips, locked or not, so selecting one selects them all; they leave any group they
        were in. Returns the group's id, or 0 when fewer than two of them exist. */
    juce::uint32 groupClips (SessionState&, const std::vector<juce::uint32>& clipIds);

    /** Breaks up every group any of these clips is in. False when none was in a group. */
    bool ungroupClips (SessionState&, const std::vector<juce::uint32>& clipIds);

    /** The clips in this clip's group, it among them; just it when it is in none. */
    std::vector<juce::uint32> clipGroup (const SessionState&, juce::uint32 clipId);

    /** Mutes or unmutes a clip, locked or not. False when it is already so. */
    bool setClipMuted (SessionState&, juce::uint32 clipId, bool muted);

    /** Gives a clip, locked or not, a colour of its own (a place in TrackColours::palette), or its
        track's with -1 or any value outside the palette. False when it already has that one. */
    bool setClipColour (SessionState&, juce::uint32 clipId, int colour);

    /** Puts take `index` of a clip in use. It keeps its place and length where the take's audio
        allows; otherwise it is cut back to what the take has. */
    bool chooseTake (SessionState&, juce::uint32 clipId, int index);

    /** Makes a clip's comp play take `index` over session frames [start, end) (cut to the
        clip), the rest as it was (its active take without a comp). False for a locked clip, a
        take out of range or a range that misses the clip, or when it already plays so. */
    bool compTake (SessionState&, juce::uint32 clipId, int index, juce::int64 start, juce::int64 end);

    /** Ends a clip's comp, take `index` in use throughout (as chooseTake). False for a locked
        clip or a take out of range, or when there is no comp and it is in use already. */
    bool playTakeThroughout (SessionState&, juce::uint32 clipId, int index);

    /** Sets the crossfade at a clip's comp joins, in frames (0 for a hard cut, at most a second
        at the session's rate). False for a locked clip, or when it is so already. */
    bool setCompFade (SessionState&, juce::uint32 clipId, juce::int64 frames);

    /** What a clip plays of its takes with its comp, before its fades and gain: as long as the
        clip, from its start; null with no comp or no audio. */
    AudioDocument::Snapshot compAudio (const SessionClip&);

    /** Makes a clip's comp (rendered by compAudio(), saved as file) a new take, in use, and ends
        the comp; its other takes stay. False for a locked clip, no comp, or audio that isn't as
        long as the clip. */
    bool flattenComp (SessionState&, juce::uint32 clipId, const juce::File&, AudioDocument::Snapshot audio);

    /** Crossfades a clip with the clips it partly overlaps on its track: where another clip ends
        inside it, its fade in and the other's fade out both span the overlap; where another starts
        inside it, its fade out and the other's fade in do. Clips inside another, or holding one,
        are left alone. False when it overlaps nothing that way. */
    bool crossfade (SessionState&, juce::uint32 clipId);

    /** Places a copy of a clip (sharing its audio and takes) on a track at position, over what
        was there, which is cleared as clearRange does. Onto a bus or past the last track, it goes
        onto a new track. Returns the copy's id. */
    juce::uint32 placeClip (SessionState&, int track, const SessionClip&, juce::int64 position);

    /** Fills from a clip's end up to `until` with copies of it laid end to end on its track (over
        what is there), the last cut short at `until`. Returns how many copies were placed. */
    int repeatClip (SessionState&, juce::uint32 clipId, juce::int64 until);

    /** The part of a clip's audio it plays, back to front. */
    AudioDocument::Snapshot reversedAudio (const SessionClip&);

    /** Makes a clip play reversed audio (from reversedAudio(), saved as file): it plays all of it
        from its start, and its fades change ends. `takes`, when given, holds every take reversed
        alike (from takeAsPlayed(), each as long as reversed; the active one's place is filled by
        reversed and file), which it keeps, lined up; otherwise its takes go. */
    bool reverseClip (SessionState&, juce::uint32 clipId, const juce::File&, AudioDocument::Snapshot reversed,
                      std::vector<SessionClip::Take> takes = {});

    /** The part of the clip's audio that plays, stretched to `ratio` times its length (within
        TimeStretch::shortest to longest) at the same pitch. Null with no audio, or when
        progress (as TimeStretch::stretch has it) cancels. */
    AudioDocument::Snapshot stretchedAudio (const SessionClip&, double ratio, double sampleRate,
                                            const std::function<bool (float)>& progress = {});

    /** The part of the clip's audio that plays, `semitones` higher or lower and just as long.
        Null with no audio, or when progress cancels. */
    AudioDocument::Snapshot pitchedAudio (const SessionClip&, double semitones, double sampleRate,
                                          const std::function<bool (float)>& progress = {});

    /** The frames of the clip's audio the Pitch Editor works on: the part it plays under the
        session's time selection, or all it plays when the selection is empty or misses it. */
    juce::Range<juce::int64> pitchRange (const SessionClip&, juce::Range<juce::int64> timeSelection);

    /** The part of `audio` (the clip's, or that with notes moved in it) that the clip plays: from
        its offset, its length. Null when audio is null or too short. */
    AudioDocument::Snapshot playedPart (const SessionClip&, const AudioDocument::Snapshot& audio);

    /** The clip as it would play another of its takes, cut to what it plays: the take's audio
        under the clip (silence where the take is shorter), from offset 0, with no other takes;
        so stretchedAudio() and pitchedAudio() can process every take alike. The clip itself
        (with no takes) for its active take or an index out of range. */
    SessionClip takeAsPlayed (const SessionClip&, int take);

    /** Makes a clip play processed audio of the part it played (from stretchedAudio() or
        pitchedAudio(), saved as file), all of it, from where it starts; its fades scale with
        its length; with keepEnd it ends where it did (moved to start earlier or later), not
        starting where it did. `takes`, when given, holds every take processed alike (from
        takeAsPlayed(), each as long as stretched; the active one's place is filled by stretched
        and file), which it keeps, lined up; otherwise its takes go. False for a locked clip, or
        takes that don't match. */
    bool stretchClip (SessionState&, juce::uint32 clipId, const juce::File&, AudioDocument::Snapshot stretched, bool keepEnd = false,
                      std::vector<SessionClip::Take> takes = {});

    /** Makes a clip play new audio for the part it plays of its active take (as long as it,
        e.g. with notes moved in the Pitch Editor, saved as file), from offset 0, keeping its
        other takes as they were, lined up with it. False for a locked clip or audio of
        another length. */
    bool replaceActiveTake (SessionState&, juce::uint32 clipId, const juce::File&, AudioDocument::Snapshot audio);

    /** Makes edited audio (Edit in Place: what the clip played, changed in the editor, saved as
        file) a new take of the clip, in use, from where the clip starts; the clip becomes as long
        as it. What it played before stays a take (a clip with no takes gets its audio as take 1);
        a comp ends, its takes staying. False for a locked clip, one that follows the tempo (whose
        original is edited instead: replaceOriginal), or no audio. */
    bool addEditedTake (SessionState&, juce::uint32 clipId, const juce::File&, AudioDocument::Snapshot audio);

    /** Gives a clip that follows the tempo a new original (its file edited, saved as file), as
        long as the old one; it is stretched again (TempoFollow::Renderer). False for a clip that
        doesn't follow, a locked one, or audio of another length. */
    bool replaceOriginal (SessionState&, juce::uint32 clipId, const juce::File&, AudioDocument::Snapshot original);

    /** Sets a clip's effects (SessionClip::Effects): its pitch shift in semitones and its chain
        (a TORQWAVE_RACK element's text, or empty for none); the render waits for the renderer.
        False for a locked clip, a shift out of range, a chain that isn't a rack, or no change. */
    bool setClipEffects (SessionState&, juce::uint32 clipId, double semitones, const juce::String& chain);

    /** Clears [start, end) on a track: clips inside it go, clips across its edges are cut back
        (or split in two) and lose their fade on the cut side. False when nothing was there. */
    bool clearRange (SessionState&, int track, juce::int64 start, juce::int64 end);

    /** Places a recorded take at position on a track (a new one when track is past the last or a
        bus), replacing what the track had there. Returns the clip's id. */
    juce::uint32 recordClip (SessionState&, int track, const juce::File& source, AudioDocument::Snapshot audio, juce::int64 position);

    /** Places the passes of a loop recording: one clip at position, length frames long, with a
        take for each pass, starting at each of offsets in the recording, the last in use. It
        replaces what the track had there. Returns the clip's id (0 with no passes). */
    juce::uint32 recordTakes (SessionState&, int track, const juce::File& source, AudioDocument::Snapshot audio,
                              juce::int64 position, juce::int64 length, const std::vector<juce::int64>& offsets);
    /** After an edit of a track in a group (before is the track as it was), the others in its
        group follow: a volume change moves theirs by as many dB (within -60 to +12, and not for a
        volume on an envelope), and mute and solo take its state. */
    void followGroup (SessionState&, const SessionTrack& before);

    /** Moves a track or bus from one place in the list to another, the others closing up behind
        it. False when either place is out of range or they are the same. */
    bool moveTrack (SessionState&, int from, int to);

    /** Puts a track or bus in a folder, or with folderId 0 takes it out. False for a folder
        (they don't nest), or when it is already so. */
    bool setTrackFolder (SessionState&, juce::uint32 trackId, juce::uint32 folderId);

    /** Makes a track an external instrument playing out of a MIDI port (an empty port: none),
        on a channel (0: the clips' own) with an offset in ms; it becomes an instrument track.
        False if nothing changed or it is a bus or a folder. */
    bool setMidiOut (SessionState&, juce::uint32 trackId, const juce::String& port, const juce::String& name, int channel, double offsetMs);

    /** Has a track play the MIDI another track's plug-ins put out (0: none). False for a bus or a
        folder, the track itself, or no change. */
    bool setMidiFrom (SessionState&, juce::uint32 trackId, juce::uint32 fromId);

    /** Binds a controller to a target, replacing what either was bound to (MIDI learn). False when nothing changed. */
    bool bindMidi (SessionState&, const MidiBinding&);

    /** Drops the bindings to a track's controls (all of them with trackId 0, the transport's too). Returns how many. */
    int unbindMidi (SessionState&, juce::uint32 trackId);

    /** Makes a track or bus join a VCA fader, or with vcaId 0 leave it. False for a folder or a
        VCA, or for something that isn't a VCA. */
    bool setTrackVca (SessionState&, juce::uint32 trackId, juce::uint32 vcaId);

    /** Sets a track's or bus's automation mode (a folder's is always Read). */
    bool setAutomationMode (SessionState&, juce::uint32 trackId, AutomationMode);

    /** Removes a track, bus or folder; removing a bus removes the sends to it, and removing a
        folder leaves its members out of any. */
    bool removeTrack (SessionState&, int track);

    /** Puts a copy of a track or bus right after it, named "Name Copy": its clips (sharing their
        audio, as unlocked copies), sends, envelopes, volume, pan, mute, solo, group, colour and
        input. Its effects are not in the state; copy them by the new id. Returns the copy's id,
        or 0 when there is no such track. */
    juce::uint32 duplicateTrack (SessionState&, int track);

    /** How long past a frozen track's last clip its effects are rendered, for tails to ring out. */
    inline constexpr double freezeTailSeconds = 5.0;

    /** Freezes a track: it plays `audio` (from frozenSource rendered, saved as file) from frame 0
        instead of its clips and effects. A 5.1 track's frozen audio is its six channels
        (fewer play as a clip of them would). False for a bus, a folder or a track that is gone. */
    bool freezeTrack (SessionState&, juce::uint32 trackId, const juce::File&, AudioDocument::Snapshot audio);
    bool unfreezeTrack (SessionState&, juce::uint32 trackId);

    /** What freezing a track renders: the track alone with its clips, pan and effects, at 0 dB
        with no volume envelope, sends or solo, unmuted; and the keys its effects' sidechains
        listen to (addKeys). A 5.1 track's is a 5.1 master, so it renders its six channels. */
    /** Brings into a session cut from another (a track or folder alone) the key tracks its
        tracks and buses listen to, by ducking or a sidechain, through any number in turn, as they
        are in the other: each with no sends, playing out of an interface pair, so it feeds its
        listeners without reaching a stereo mix; its folder's mute and volume taken on. */
    void addKeys (const SessionState& from, SessionState& alone);

    SessionState frozenSource (const SessionState&, juce::uint32 trackId);

    /** Moves each clip on a track that starts after the end of the clips before it back to that
        end, from the first clip on, so they play one after another. A clip overlapping the ones
        before it moves with them, keeping the overlap. Locked clips stay where they are, and the
        ones after them close up to them. Returns how many clips moved. */
    int closeGaps (SessionState&, int track);

    /** Moves each of these clips, on its own track, so that it starts on the grid line nearest
        its start (nearestLine gives it). Locked clips stay. Returns how many moved. */
    int quantizeClips (SessionState&, const std::vector<juce::uint32>& clipIds, const std::function<juce::int64 (juce::int64)>& nearestLine);

    /** The same for note clips: each moved on its track to nearestLine (its start), never before 0. Returns how many moved. */
    int quantizeNoteClips (SessionState&, const std::vector<juce::uint32>& clipIds, const std::function<juce::int64 (juce::int64)>& nearestLine);

    /** Clears [start, end) on every track, hidden ones too, as clearRange and clearNoteRange do.
        False when nothing was there. */
    bool clearTime (SessionState&, juce::int64 start, juce::int64 end);

    /** Takes [start, end) out of the session's time. On every track, hidden ones too, it is
        cleared as clearRange does, and the clips, envelope points and markers after it move back
        by its length; envelopes keep their shape on both sides, and markers follow as
        MarkerEdits::afterReplace has them. Locked clips stay as they are, where they are. False
        when nothing changed. */
    bool deleteTime (SessionState&, juce::int64 start, juce::int64 end);

    /** How removing clips closes the time they leave: not at all, on their own tracks, or on every track. */
    enum class Ripple { off, track, allTracks };

    /** Where a removed clip was: its track and the session time it played over. */
    struct RemovedSpan
    {
        int track = 0;
        juce::int64 start = 0, end = 0;
    };

    /** After clips were removed, closes the time they left empty, the parts of it no clip still
        plays over. Ripple::track: on each clip's own track, its later clips, note clips and
        envelope points move back. Ripple::allTracks: the empty time (empty on every track) is
        taken out of the session as deleteTime does, markers and all. Locked clips stay where
        they are. False when nothing moved. */
    bool rippleRemoved (SessionState&, const std::vector<RemovedSpan>&, Ripple);

    /** Puts the clips on top of the others on their tracks (last in the list), or under them
        (first), keeping their order among themselves. False when none moved. */
    bool bringClipsToFront (SessionState&, const std::vector<juce::uint32>& clipIds);
    bool sendClipsToBack (SessionState&, const std::vector<juce::uint32>& clipIds);

    /** Puts `length` frames of empty time into the session at `at`. On every track, hidden ones
        too, clips across it are split there, and the clips, envelope points and markers from it on
        move later by `length`; envelopes hold their value at `at` over the new time. Locked clips
        stay as they are, where they are. False when nothing changed. */
    bool insertTime (SessionState&, juce::int64 at, juce::int64 length);

    /** Puts a tempo change at its bar (after the first; the first bar's tempo is the session's),
        replacing one there. Tempo 20 to 400, 1 to 16 beats in a bar; false outside those or when
        it is already so. Clips and markers stay where they are in time. */
    bool setTempoChange (SessionState&, Metronome::TempoChange);
    bool removeTempoChange (SessionState&, juce::int64 bar);

    /** Sets the session's tempo (20 to 400) and beats in a bar (1 to 16) from the start; false
        outside those or when they are as given. The clips on musical tracks keep their bars and
        beats through this, setTempoChange and removeTempoChange (see SessionTrack::musical). */
    bool setTempo (SessionState&, double tempo, int beatsPerBar);

    /** Replaces the tempo, time signature and every tempo change at once (as setTempo and
        setTempoChange check them, changes sorted, one a bar, each after the first), keeping the
        clips on musical tracks on their bars and beats. False when one is out of range or it is
        all as given. */
    bool setTempoMap (SessionState&, double tempo, int beatsPerBar, std::vector<Metronome::TempoChange>);

    /** Sets the session's key from a key as written ("A minor", "Am", "F# dorian"), stored as
        its name ("A minor"); empty clears it. False when it isn't a key or it is already so. */
    bool setKey (SessionState&, const juce::String& key);

    /** Copies [start, end) from every track: the clips across it cut to it (losing their fades
        on the cut sides), each envelope's line across it, and the markers in it as
        MarkerEdits::afterCrop has them. Empty (length 0) when the range is. */
    SessionTime copyTime (const SessionState&, juce::int64 start, juce::int64 end);

    /** Puts copied time in at `at`: insertTime makes room, then each track's clips go onto the
        track at the same place in the list (as unlocked copies with new ids; tracks past the last
        are left out), its envelope lines replace the held values there, and its markers are
        added. A volume or pan line pasted onto a track without an envelope gives it one that
        stays at the track's setting outside the pasted time; an effect parameter's line is pasted
        only where the track already has that parameter's envelope. Where the copy had no line,
        the envelope holds its value, as insertTime does. False when the time is empty or at
        another sample rate. */
    bool pasteTime (SessionState&, const SessionTime&, juce::int64 at);

    //==============================================================================
    // The arranger. Section edits that move time (move, copy, delete, insert bars) do it as the
    // time edits above do, everything on every track with them: clips, note clips, envelopes,
    // markers, sections, and tempo and time-signature changes when the section is whole bars.

    /** Adds a section over [start, end), named so (or "Section n" when empty); it must overlap
        no other. Returns its id, or 0. */
    juce::uint32 addSection (SessionState&, juce::int64 start, juce::int64 end, const juce::String& name = {});

    /** Renames a section, or gives it a colour (-1: the strip's). False when it is so already. */
    bool renameSection (SessionState&, juce::uint32 id, const juce::String& name);
    bool setSectionColour (SessionState&, juce::uint32 id, int colour);

    /** Moves a section's edges, leaving what is in the session where it is; it must not overlap
        another. */
    bool resizeSection (SessionState&, juce::uint32 id, juce::int64 start, juce::int64 end);

    /** Takes a section off the strip, leaving what was in it. Its places in the chain go. */
    bool removeSection (SessionState&, juce::uint32 id);

    /** Moves (or, with copy, copies) a section and everything in it so that it starts at `to` in
        the session as it will be: the time it leaves closes up (a move) and the time at `to`
        opens for it. `to` may not be inside the section; locked clips in it stay where they are.
        A copy is a new section after it in the list, named as the section. */
    bool moveSection (SessionState&, juce::uint32 id, juce::int64 to, bool copy);

    /** Takes a section and its time out of the session, closing the gap (deleteTime). */
    bool deleteSection (SessionState&, juce::uint32 id);

    /** Puts `bars` empty bars in at a frame, at the tempo there (insertTime): a section starting
        there moves later; one ending there stays. Or before a section (at its start). */
    juce::int64 insertBars (SessionState&, juce::int64 at, int bars);   ///< The frames put in, 0 if none.
    bool insertBarsBefore (SessionState&, juce::uint32 id, int bars);

    /** Sets the arrangement chain (unknown ids dropped), and whether playback follows it. */
    bool setArrangement (SessionState&, std::vector<juce::uint32> chain);
    bool setPlayArrangement (SessionState&, bool on);

    /** Makes the chain the session's time: from the first section's start to the last one's end,
        the time is replaced by each section of the chain in order, as pasteTime puts them; the
        chain is then each new section once, and playing it off. False with no chain. */
    bool flattenArrangement (SessionState&);

    //==============================================================================
    // Mixer scenes (MixScene).

    /** Stores the mix as it is now as a new scene, named so (or "Scene n" when empty). Returns its id. */
    juce::uint32 storeScene (SessionState&, const juce::String& name = {});

    /** Stores the mix as it is now into a scene, which keeps its name. False when there is no such
        scene, or it holds this mix already. */
    bool updateScene (SessionState&, juce::uint32 id);

    /** Sets the mix of every track the scene holds that is still in the session to the scene's;
        its sends to buses still there replace the track's sends. False when there is no such
        scene, or the mix is the scene's already. */
    bool recallScene (SessionState&, juce::uint32 id);

    /** Renames a scene (not to nothing); false when it is so already. */
    bool renameScene (SessionState&, juce::uint32 id, const juce::String& name);
    bool removeScene (SessionState&, juce::uint32 id);

    /** Sends a track's or bus's output to a bus in place of the mix, or with busId 0 back to the
        mix. False for a folder, a destination that isn't a bus, a bus into itself or into a bus
        that already plays into it (a loop of any length), or when it is already so. */
    bool setTrackOutput (SessionState&, juce::uint32 trackId, juce::uint32 busId);

    /** Sends a track's or bus's output out of a pair of the interface's outputs (from firstChannel)
        in place of the mix, or with -1 back to the mix. False for a folder, a channel out of
        [0, maxDeviceOutputs - 1), or when it is already so. */
    bool setTrackDeviceOutput (SessionState&, juce::uint32 trackId, int firstChannel);

    /** Feeds a key track's output into the sidechain of the effect in this slot of a track or
        bus (a key of 0 stops it). False for a folder, a key that isn't another track, a key that
        listens to this track itself (through any keys), or when it is already so. */
    bool setSidechain (SessionState&, juce::uint32 trackId, juce::uint32 slotId, juce::uint32 keyTrackId);

    /** Sets a track's ducking (a key of 0 turns it off). False for a bus or folder, a key that
        isn't another track, settings out of range, or a key that ducks under this track itself,
        however many steps away. */
    bool setDucking (SessionState&, juce::uint32 trackId, const Ducking&);

    /** Makes a track a 5.1 track or a stereo one. False for a bus, folder or frozen track, a track
        that is gone, or when it is already so. */
    bool setTrackFiveOne (SessionState&, juce::uint32 trackId, bool fiveOne);

    /** Makes a track stereo (2), 5.1 (6) or 7.1 (8); false for other counts, buses, folders,
        frozen tracks and no change. */
    bool setTrackChannels (SessionState&, juce::uint32 trackId, int channels);

    /** Makes the master 5.1 or stereo. False when it is already so. */
    bool setSurround (SessionState&, bool fiveOne);

    /** Makes the master stereo (2), 5.1 (6) or 7.1 (8). False for another count, or when it is already so. */
    bool setMasterChannels (SessionState&, int channels);

    /** Places a track or bus in a 5.1 master; x, y are kept to [-1, 1], width to [0, 1] and the LFE to
        [-60 dB, maxLfeDb] or off (anything below -60). False for a folder, a track that is gone,
        or when it is already so. */
    bool setSurroundPan (SessionState&, juce::uint32 trackId, SurroundPan);

    /** Sets a track's send to a bus, adding it if there is none. */
    bool setSend (SessionState&, juce::uint32 trackId, const SessionSend&);
    bool removeSend (SessionState&, juce::uint32 trackId, juce::uint32 busId);

    /** Replaces a track's volume or pan envelope; its points are kept in range and in time
        order. An empty envelope hands the parameter back to the track's setting. */
    bool setEnvelope (SessionState&, juce::uint32 trackId, const AutomationTarget&, Envelope);

    /** Writes a pass of automation recorded while playing: points in time order. From the pass's
        first frame to returnFrames after its last, the envelope's old points give way to the
        pass, which starts from the old value there and ends by going back, over returnFrames,
        to the old value. Outside that, the envelope (or, with none, the track's setting, or for
        an effect parameter the value the pass starts from) stays as it was. */
    bool writeAutomation (SessionState&, juce::uint32 trackId, const AutomationTarget&,
                          const std::vector<AutomationPoint>& pass, juce::int64 returnFrames);

    /** A note clip's values kept to what plays: the position and offset from 0, the length at
        least 1, the loop length from 0 (once) up to offset + length, pitches 0 to 127, each note's
        start from 0 and length at least 1, velocities above 0 and up to 1, the name "Notes" when
        empty; lanes with a controller 0 to 128, one a controller, points from 0 to 1 in time
        order, empty ones dropped. */
    NoteClip tidiedNoteClip (NoteClip);

    /** Adds a note clip (its id given here, the rest tidied as above) to a track: not a bus, a
        folder or a frozen track. Returns its id, or 0 when it can't be added. */
    juce::uint32 addNoteClip (SessionState&, juce::uint32 trackId, NoteClip);

    /** Replaces a note clip's name, length, loop length, notes, lanes and colour (tidied as
        above); it keeps its id, track, position and mute. False when there is no such clip. */
    bool replaceNoteClip (SessionState&, juce::uint32 clipId, const NoteClip&);

    /** Moves a note clip to a track (a new one when past the last; onto a bus or folder it only
        moves in time) and a position (not before 0). False when there is no such clip. */
    bool moveNoteClip (SessionState&, juce::uint32 clipId, int toTrack, juce::int64 position);
    bool removeNoteClip (SessionState&, juce::uint32 clipId);
    bool setNoteClipMuted (SessionState&, juce::uint32 clipId, bool muted);

    /** Sets where a note clip starts and ends in the session, its notes staying where they are in
        time (the start not before the notes' origin, so the offset stays from 0; at least one
        frame long). False when there is no such clip. */
    bool trimNoteClip (SessionState&, juce::uint32 clipId, juce::int64 start, juce::int64 end);

    /** Splits a note clip in two at a session frame strictly inside it, both parts playing the
        notes they did. */
    bool splitNoteClip (SessionState&, juce::uint32 clipId, juce::int64 frame);

    /** Places a copy of a note clip on a track at position; onto a bus, folder or frozen track,
        or past the last track, it goes onto a new track. Returns the copy's id. */
    juce::uint32 placeNoteClip (SessionState&, int track, const NoteClip&, juce::int64 position);

    /** Stretches a note clip by a ratio (1/16 to 16): its notes, lanes, loop and length scale
        with it, keeping its start, or its end when keepEnd. False when there is no such clip
        or the ratio is not a number above 0. */
    bool stretchNoteClip (SessionState&, juce::uint32 clipId, double ratio, bool keepEnd);

    /** Moves every note of the note clips up (or, negative, down) by semitones, each kept to
        0 to 127. Returns how many clips changed. */
    int transposeNotes (SessionState&, const std::vector<juce::uint32>& clipIds, int semitones);

    /** Sets every note's velocity in the note clips to `to` (1 to 127, kept as a fraction of
        127), or with `to` 0 scales each by `scale`, kept to 1 to 127. Returns how many clips changed. */
    int setNotesVelocity (SessionState&, const std::vector<juce::uint32>& clipIds, int to, double scale);

    /** Moves each note's start in the note clips to nearestLine (a session frame in, the line's
        session frame out), its length kept, never before the clip's timeline start. Returns how
        many clips changed. */
    int quantizeNotes (SessionState&, const std::vector<juce::uint32>& clipIds, const std::function<juce::int64 (juce::int64)>& nearestLine);

    /** Quantize Notes on the session's tempo map, by `q`: each note's start goes `strength` of
        the way to the nearest line of the (swung) grid, never before the clip's timeline start;
        with `lengths`, its end does too, at least one grid step after its start. */
    int quantizeNotes (SessionState&, const std::vector<juce::uint32>& clipIds, const NoteQuantize& q);

    /** The notes and lane points a clip plays, laid out once from its start (every pass of a
        repeating clip written out, notes cut at its end, each lane starting with its value at the
        clip's start): the same clip with no offset and no loop. */
    NoteClip flattenedNoteClip (const NoteClip&);

    /** Joins two note clips on one track into one, from the earlier start to the later end, that
        plays what both did (each flattened); it keeps clipId, and the earlier clip's name and
        colour; the other clip goes. False when they are not two note clips on one track. */
    bool mergeNoteClips (SessionState&, juce::uint32 clipId, juce::uint32 otherId);

    /** Clears [start, end) of a track's note clips: clips inside it go, clips across its edges
        are cut back (or split in two), their notes staying in time. False when nothing was there. */
    bool clearNoteRange (SessionState&, int track, juce::int64 start, juce::int64 end);
}

//==============================================================================
/** Snapping: edits that land near something line up with it exactly. */
namespace SessionSnap
{
    /** What a dragged edge can snap to: the session start, the extra frames given (the cursor,
        a time range's edges), every clip's start and end but the ignored clip's, and a grid: with
        a beat grid, its lines on the session's tempo map (the metronome's beats), else with a grid
        step, every multiple of it. */
    struct Targets
    {
        std::vector<juce::int64> extra;
        juce::uint32 ignoreClip = 0;
        std::vector<juce::uint32> ignoreClips;   // more clips being dragged
        int ignoreMarker = -1;             // the index of a marker being dragged
        juce::int64 gridStep = 0;
        std::optional<Metronome::Grid> beatGrid;
        bool toGrid = false;               // with nothing within tolerance, the nearest grid line anyway
    };

    /** The target nearest frame, when one is within tolerance frames; otherwise, with toGrid,
        the nearest grid line; otherwise frame. */
    juce::int64 snap (const SessionState&, juce::int64 frame, juce::int64 tolerance, const Targets&);

    /** For a span being moved: how far to shift it so that its start or its end (whichever is
        nearer to a target) lands on one; 0 when neither is within tolerance. */
    juce::int64 snapSpan (const SessionState&, juce::Range<juce::int64> span, juce::int64 tolerance, const Targets&);
}

//==============================================================================
/** The open multitrack session: current state, undo history and dirty state, the multitrack
    counterpart of AudioDocument. Edit methods must be called on the message thread; getState()
    may be called from any thread. */
class Session final : public juce::ChangeBroadcaster
{
public:
    Session();

    using State = std::shared_ptr<const SessionState>;

    /** Replaces everything (a new or loaded session); clears undo history. */
    void reset (SessionState initial, const juce::File& file);

    /** Gives the session a master track when it has none (one from before them, or a new one),
        in every step of its history alike, so undo and redo keep it; not a step of its own.
        Returns its id. */
    juce::uint32 ensureMaster();

    State getState() const;
    juce::uint32 getVersion() const noexcept      { return version.load(); }
    double getSampleRate() const noexcept         { return sampleRate.load(); }
    juce::File getFile() const                    { return file; }

    /** Applies a new state as one undoable step. With merge, a step with the same description
        as the last one replaces it instead (a slider being dragged, say). */
    void apply (SessionState next, const juce::String& description, bool merge = false);

    bool canUndo() const noexcept                 { return position > 0; }
    bool canRedo() const noexcept                 { return position + 1 < (int) history.size(); }
    juce::String getUndoDescription() const;
    juce::String getRedoDescription() const;
    void undo();
    void redo();

    /** Every step's description, oldest first: the first is where the session started (no
        description), and getPosition() is the one it is at. Steps after it can be redone. */
    juce::StringArray getHistory() const;
    int getPosition() const noexcept              { return position; }

    /** Goes back or forward to a step in getHistory(), as that many undos or redos would. */
    void goTo (int step);

    bool isDirty() const noexcept                 { return position != savedPosition || effectsChanged || viewChanged; }

    /** The loop and the ruler's bars and beats, as a transport's locators are: saved with the
        session but not undo steps, and an undo or redo leaves them as they are. */
    void setLoop (juce::Range<juce::int64>);
    void setBarsAndBeats (bool);

    /** Tracks' automation modes, likewise kept out of the undo steps, as in Cubase: an undo
        leaves every track in the mode it is in, and apply() keeps each existing track's mode
        (a new track brings its own). trackId 0: every track and bus. */
    void setAutomationMode (juce::uint32 trackId, AutomationMode);
    void markSaved (const juce::File& savedAs);

    /** Marks the session as having changes to save (a recovered copy, say), at every step. */
    void markUnsaved();

    /** Track effects live outside the undoable state; changing them still needs saving. */
    void markEffectsChanged();

    /** Puts in a state that only brings the current one up to date (audio stretched to the tempo,
        TempoFollow::Renderer): in its place, not a step of its own, and not a change to save. */
    void refresh (SessionState);

    /** Frees states dropped from history once the audio thread has let go of them. */
    void releaseUnusedStates();

private:
    struct Entry
    {
        State state;
        juce::String description;
    };

    void setCurrent (const State&);

    /** The state with this session's loop, bars and beats and automation modes, as setLoop,
        setBarsAndBeats and setAutomationMode keep them. */
    State withViewOf (State, const SessionState& now) const;
    void replaceCurrent (SessionState);

    mutable juce::SpinLock lock;
    State current;
    std::atomic<juce::uint32> version { 0 };
    std::atomic<double> sampleRate { 48000.0 };

    std::vector<Entry> history;
    int position = 0;
    int savedPosition = 0;
    bool effectsChanged = false;
    bool viewChanged = false;   // the loop or bars and beats, since the last save
    std::vector<State> pendingRelease;
    juce::File file;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Session)
};

//==============================================================================
/** Session files (.tqsession): XML naming each clip's audio file, relative to the session
    file where it can. */
namespace SessionFile
{
    /** trackEffects gives each track's EFFECTS element (or null), stored inside its TRACK. */
    juce::String save (const SessionState&, const juce::File& target,
                       const std::function<std::unique_ptr<juce::XmlElement> (juce::uint32 trackId)>& trackEffects = {});

    struct Loaded
    {
        SessionState state;
        juce::String error;
        juce::StringArray missing;   // audio files that could not be read; their clips are left out
        std::map<juce::uint32, std::shared_ptr<juce::XmlElement>> effects;   // EFFECTS elements, by new track id
    };

    /** Each audio file is read once, however many clips use it. Files at another sample rate
        are converted to the session's. */
    Loaded load (const juce::File&, juce::AudioFormatManager&);

    /** A track's envelopes (volume, pan, surround pan, effect parameters; none that are empty) as
        ENVELOPE children of into, as a session file holds them. */
    void writeEnvelopes (const SessionTrack&, juce::XmlElement& into);

    /** Sets the track's envelopes from from's ENVELOPE children, each point's frame times
        frameScale (rounded; for envelopes made at another sample rate). */
    void readEnvelopes (SessionState&, juce::uint32 trackId, const juce::XmlElement& from, double frameScale = 1.0);
}
