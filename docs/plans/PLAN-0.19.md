# 0.19 plan: the editor and DAW in one

0.19 is the first line of work toward Torqwave as an editor and a DAW in one, for **sound design**, **writing and producing music**, and **working to picture** (see the [roadmap](../../ROADMAP.md)). This plan scopes each piece:

- what it is;
- what it builds on in the code today;
- its size: **S** is days, **M** about a week, **L** more;
- how it is tested;
- the order it is built in.

Each piece lands as its own commit with its tests and docs. The [changelog](../../CHANGELOG.md) records each one as it is built.

## What the code gives us to build on

- **Session state is an immutable snapshot.** `SessionState` is shared as `shared_ptr<const SessionState>`. An edit copies the state, changes it through a `SessionEdits` function, and applies it with `Session::apply` as one undo step, merged for drags. The audio thread picks up the new state by version. Every feature below follows this pattern, so undo comes for free.
- **Mixing:**
  - `SessionMixer` mixes the whole session inside the player. Tracks are not nodes in the `AudioProcessorGraph`.
  - Each track's chain is a `TrackEffects`, with the instrument in its first slot.
  - Delay compensation reads tracks ahead and holds outputs back.
- **Time:**
  - The tempo map (`Metronome::TempoMap`) allocates nothing and is safe on the audio thread. Its changes sit at bar starts.
  - Clips, notes and envelopes are stored in sample frames.
  - A *musical* track keeps its clips on their beats when the tempo changes. Note clips stretch with it; audio clips only move.
- **Takes:** a clip holds `takes` and one `activeTake`. Every take reads the same file at its own `shift`, so trims and splits act on all of them at once.
- **Automation:**
  - Envelopes are linear point lists, read per sample for volume and pan, and every 64 frames for plug-in parameters.
  - A global Write Automation switch writes a pass from the track header or a plug-in window.
  - `SessionEdits::writeAutomation` merges a pass into the old points.
- **MIDI:**
  - MIDI input reaches one target track through `MidiIn::Live`.
  - Note clips send on channel 1.
  - Nothing ever opens a `MidiOutput`.
- **Time edits across tracks** already exist and move markers and envelopes: `deleteTime`, `insertTime`, `copyTime` and `pasteTime`. They leave tempo changes and the loop alone.
- **Video:**
  - The session stores a video and its start in seconds.
  - Timecode display has 24, 25, 29.97 drop-frame, 29.97 and 30 fps.
  - The session does not store a frame rate, a start timecode or 23.976.
- **Detection and stretching:**
  - The only onset detector is `PhaseVocoder::onsets()` (spectral flux).
  - Clip stretching is rendered offline into a new file. Nothing stretches in real time.

## The pieces, in build order

### 1. Comping (M)
Build one performance from parts of several takes, as in Cubase and Logic.

- **Model:** `SessionClip` gains `comp`: a sorted list of `{start, take}` segments across the clip. Each segment plays its take, read at that take's `shift`. An empty list means the active take throughout, which is today's behaviour. Joins get a short equal-power crossfade, 10 ms by default and set per clip. Saved in the session file; old files load unchanged.
- **Playback:** `SessionMixer::readClip` reads each segment's take and crossfades at the joins. It allocates nothing on the audio thread.
- **View:**
  - **Show Takes** on a clip, or for a track, opens its take lanes under the clip lane, one row per take.
  - Dragging across a take lane swipes that range into the comp, as one undo step.
  - Clicking a take lane's name makes that take play throughout.
  - The comp is drawn on the clip, with the joins marked.
- **Edits:** splitting, trimming, moving, stretching and reversing keep the comp. **Flatten Comp** renders it into one take.
- **Ask Torq:** gains a `comp_takes` tool.
- **Tests:**
  - A comp of three takes plays sample-exact against reading each take by hand.
  - Joins are crossfaded.
  - The comp survives a save and load, undo, a split and a trim.
  - Flatten gives what playback gave.

### 2. Arranger: sections across every track (M)
- **Model:** `SessionState` gains `sections`: named, coloured spans on an **Arranger** strip above the marker lane.
- **Editing sections:**
  - Draw a section by dragging the strip; double-click its name to rename it.
  - Drag a section to move it with everything inside it: clips, notes, envelopes, markers, and tempo and time-signature changes.
  - Alt-drag copies a section.
  - Delete a section and close the gap.
  - Insert empty bars before a section.
  - All of these build on `copyTime`, `pasteTime`, `deleteTime` and `insertTime`, which are extended to carry `tempoChanges` and the loop.
- **Arrangement chain:** an optional play order (Intro, Verse, Chorus, Verse…) that playback follows. It is rendered by **Flatten Arrangement**.
- **Ask Torq:** "double the chorus", "make the intro 4 bars shorter".
- **Tests:**
  - Moving, copying and deleting a section carries clips, notes, envelopes, markers and tempo changes exactly, with undo.
  - The arrangement chain plays the right bars.
  - Sections save and load.

### 3. Automation modes, mixer writing and VCA faders (M)
- **Modes:** each track and bus gets Off, Read, Touch, Latch or Write, replacing the global switch, which becomes "all tracks to Write/Latch".
  - **Touch** writes while a control is held, then returns to the envelope.
  - **Latch** holds the last value until stop.
  - **Write** overwrites from play to stop.
  - **Off** ignores the envelope.
  - All of these build on `startAutomationPass`, `continueAutomationPass` and `writeAutomation`.
- **The mixer writes:** its faders and pan knobs write automation too, not only the track header.
- **VCA faders:** a VCA track kind with no audio, which tracks join instead of a bus.
  - Its fader, and its own envelope, offset every member's volume in dB at mix time, without summing them.
  - Mute and solo follow the VCA.
  - Groups stay as they are.
- **Tests:**
  - Each mode writes or plays what it should over a scripted pass.
  - A VCA moves its members' levels sample-exactly, its envelope too.
  - Modes and VCAs save and load.

### 4. Drum grid, swing and retrospective MIDI (M)
- **Drum grid:** a step-grid view of a note clip in the Piano Roll.
  - It shows one row per drum sound, named from a drum map (General MIDI by default, editable, saved with the track).
  - Steps are set by clicking, with velocity by drag.
- **Swing:**
  - Swing on the grid and in Quantize Notes: 50–75 %, on 1/8 or 1/16.
  - Quantize gains strength and lengths.
- **Retrospective MIDI record:** while stopped or playing, the last 60 s of incoming MIDI is kept. **Capture MIDI** makes a note clip of it at its tempo.
- **MIDI channels:** note clips and controller lanes gain a channel (1–16), for multi-timbral instruments and MIDI out.
- **Tests:**
  - The grid places exactly the notes a step pattern means.
  - Swing moves the off-beats by the set amount.
  - A capture after playing while stopped holds those notes at their times.

### 5. MIDI out (M)
- **External instrument tracks:** a track's output can be a MIDI port (`juce::MidiOutput`) and channel, so its note clips, controller lanes and live input play a hardware synth. Delay is compensated by a set offset.
- **MIDI from plug-ins:** MIDI a plug-in outputs (an arpeggiator, a chord or MIDI effect) can feed another instrument track, routed in `TrackEffects`.
- **MIDI clock and transport out:** start, stop, continue and song position.
- **Tests:**
  - A virtual MIDI port (macOS and Linux) receives exactly the clip's messages at the right times.
  - Plug-in MIDI reaches the second track.

### 6. Audio that follows the tempo (L)
- **Real-time stretching:** a per-clip stretcher in `SessionMixer::readClip`, using Signalsmith Stretch in real time, reading ahead and compensated like plug-in delay. It is used only for clips that need it.
- **Musical audio clips:**
  - On a musical track, a clip gets a source tempo, set by hand, read from the file name or detected.
  - It stretches to the session tempo map, including tempo changes inside the clip.
- **Warp markers:**
  - Pins in a clip that fix a point of the audio to a beat.
  - Set by hand or from detected onsets (`PhaseVocoder::onsets()`, exposed).
  - Dragged in the clip to tighten a performance.
- **Slicing:** **Slice at Transients** splits a clip at its onsets, for drum loops.
- **Tempo detection:** a tempo estimate for a clip, from its onsets.
- **Tests:**
  - A click track at 100 BPM played in a 120 BPM session lands on the 120 grid, within 1 ms.
  - Warp markers pin their points exactly.
  - Stretching costs no allocation on the audio thread.
  - Rendering and playback agree.

### 7. Control surfaces (M)
- **Mackie Control** over a MIDI port pair, with the HUI mode many surfaces have:
  - eight-fader banks;
  - V-pots for pan;
  - mute, solo, select and record arm;
  - transport;
  - the scribble strips;
  - motorised faders that follow automation.
- **Generic MIDI learn:** for any knob on any controller.
- **Tests:** a simulated Mackie device on a virtual port checks the messages both ways, against the protocol.
- **Not tested on hardware** until a surface is tried; the docs will say which.

### 8. Working to picture (M)
- **Session timecode:**
  - The session gains a frame rate (23.976, 24, 25, 29.97 drop-frame and non-drop, 30) read from the video, and a start timecode (for example 01:00:00:00).
  - The clock, the ruler and markers show timecode.
- **Cut detection:** **Find Cuts** runs ffmpeg's scene detection on the video and adds markers at the cuts. Hit points can be marked by hand with a key while playing.
- **Fit Tempo to Hits:** choose the hit markers that should land on beats or bars, and Torqwave finds the tempo, or a tempo map, that puts them closest. It shows how far off each hit lands.
- **Spotting:** a Sound Library sound dropped on a marker lands with its sync point on that marker.
- **Tests:**
  - Timecode round trips at every rate, drop-frame included.
  - Cut detection finds the cuts of a generated test video.
  - Fit Tempo puts given hits within a frame.

### 9. Sound design in the session (M), the part only Torqwave has
- **Edit a clip in place:** opens that clip's audio in the waveform and spectral editor as a linked document. Every editor tool works on it: spectral selection and attenuation, healing, repair, noise reduction, the Effects menu and Ask Torq.
  - Saving writes a new take and the clip switches to it (`replaceActiveTake`), so the original stays one take away.
  - Today, double-clicking opens the whole source file with no way back to the clip.
- **Clip effects:** a non-destructive effect chain on a single clip, as Cubase's clip effects, rendered on demand or played live.
- **Layering:**
  - **Combine to New Sound:** renders a selection of stacked clips (layers, pitch, effects) into one sound file.
  - **Add to Library:** files that sound in the Sound Library with its category and tags.
- **Tests:**
  - An in-place edit replaces just that clip's audio, as a new take, with undo.
  - Clip effects play and render the same.

### 10. Smaller pieces (S each)
- **Mixer snapshots / scenes:** store and recall every track's mix, with undo.
- **Channel-strip presets:** a track's effects, sends and settings, applied to an existing track. This builds on `TrackTemplate`, which today only makes new tracks.
- **Hardware inserts:** an effect slot that sends to an interface output pair and returns from an input pair. A ping measures its round trip, which delay compensation then uses.
- **Retrospective audio record:** not planned; it needs always-on recording.
- **Docs fix:** the Sound Library guide says Return inserts into the editor; it inserts into the session.

## Order and releases

The pieces are built in the order above:

1. **First, comping, the arranger and automation modes:** the everyday work of recording and arranging.
2. **Then the drum grid, swing, retrospective MIDI and MIDI out:** writing music.
3. **Then audio that follows the tempo,** the biggest and riskiest piece.
4. **Then control surfaces, picture work and sound design in the session.**

0.19.0 ships when pieces 1 to 5 are in, tested and documented. Later pieces can land in 0.19.x releases as each one is done. Each release says plainly what was tried by hand and what was not.
