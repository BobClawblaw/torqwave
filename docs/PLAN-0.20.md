# 0.20 plan: the sequencer and the sequence library

0.20 adds a sequencer to Torqwave: a library of MIDI parts, each with the instrument that plays it, to preview, edit, and drop into a session at its tempo. You build up drums, bass, pads, arps, harmony, melodies and effects as you work, then pull parts out of the library to try musical ideas and put a new song together quickly.

Each piece below has:

- what it is;
- what it builds on in the code today;
- its size: **S** is days, **M** about a week, **L** more;
- how it is tested.

Each step lands as its own commit with its tests, guide pages and a changelog entry that says what was not tried by hand. The steps are built in order: each one is useful on its own.

## What the code gives us to build on

- **Note clips** (`NoteClip`): notes (`ClipNote`: pitch, start, length, velocity) and controller lanes at frames of the clip's own timeline, which starts `offset` frames before the clip. A loop length repeats the notes. A note clip plays into the first slot of its track's chain when that slot is an instrument.
- **Musical tracks** keep their clips on their beats when the tempo changes. New instrument tracks are musical.
- **The tempo map** (`Metronome::TempoMap`) turns frames into beat positions and back (`beatPosition`, `frameAtBeatPosition`), through tempo changes.
- **Track templates** (`TrackTemplate`, `.sbtrack`): a track's kind, volume, pan, colour and effects chain (instrument first, with each plug-in's state) as XML. `TrackTemplate::add` makes a track from one, and `AudioEngine::loadTrackEffects` loads its plug-ins in the background.
- **Editing notes:** the Piano Roll (`NoteEditor`), the drum grid and the controller lanes already edit a note clip in place.
- **MIDI files:** `MidiIn::importFile` reads a Standard MIDI File into note clips.
- **The Sound Library** has a browser, preview, ratings, favourites, tags and drag-and-drop into the session, all for audio files.

## The unit: a sequence (`.tqseq`)

One file holds everything that is needed to play a part anywhere:

- **Notes in beats, not frames:** each note's start and length in ticks, 960 to a beat, and its pitch and velocity. Controller lanes the same way. The part's length and its loop length are in ticks too, with its beats in a bar. Because nothing is in frames, a sequence lands at any session tempo exactly, with nothing stretched.
- **The voice:** a track template of the track it came from: its instrument with its state, its effects, volume, pan and colour. Sends, the output, inputs and automation are left out, since they belong to a session.
- **What it is:** its name, role and sub-role (the library's first two levels), key and scale, the tempo it was written at (for reference only), tags and a note.

The file is XML, like a session, so it can be read, diffed and repaired by hand.

## The library

The first two levels are fixed roles, as UCS categories are for audio. Below them, the folders are the user's own.

```text
Sequences/
├── Drums/        Full Grooves · Kick · Snare & Clap · Hats & Tops · Percussion · Fills · Breaks
├── Bass/         Sub · Synth Bass · Arp Bass · Picked & Played
├── Harmony/      Progressions · Keys & Comping · Stabs · Strums
├── Pads/         Sustained · Evolving · Swells · Drones
├── Arps/         Up & Down · 16th Sequences · Plucks · Gated
├── Melody/       Leads · Hooks · Counter-lines · Bells & Mallets
├── SFX/          Risers · Downlifters · Impacts · Sweeps · Transitions · Hits
├── Kits/         parts meant to play together, a folder each
└── Inbox/        new sketches, not yet sorted
```

On disk it is the same tree, by default under `~/Music/Torqwave/Sequences/`, as plain files to copy, back up and share.

## Step 1: the format, the library browser, saving and dropping (M)

- **`SequenceFile`:** writes and reads `.tqseq`; turns a note clip in a session into beats (through the session's tempo map, from where the clip starts) and back into a note clip at any frame of any session (through that session's map).
- **`SequenceLibrary`:** the folder, its fixed roles and sub-roles (made when missing), and a scan of the `.tqseq` files under it, each with its name, role, path, key, bars and tags.
- **Add to Sequence Library...** (a note clip's menu, and Multitrack > Clips): a dialog for the name, role, sub-role, key and tags, then saves the clip with its track's instrument and effects. The clip's loop and lanes go with it.
- **The Sequence Library panel:** a tree of the roles, sub-roles and the user's folders, with a search box. Each sequence shows its bars and key. Right-click to reveal it in Finder, rename it or delete it (to the Trash).
- **Dropping into the session:**
  - on empty space below the tracks: a new instrument track made from the voice, with the clip on the grid where it was dropped;
  - on an instrument track: the notes only, played by that track's instrument; with Option, a new track from the voice instead;
  - from Finder too: a `.tqseq` file dropped on the session does the same.
- **Tests:** a clip saved and read back keeps every note, lane and loop exactly in beats; one written at 90 BPM and dropped into a 140 BPM session lands on the same beats, and through a tempo change too; the voice comes back with its instrument and effects, without sends or automation; the scan finds the roles, sub-roles and the user's folders, and skips files that are not sequences; a drop on an instrument track adds notes only.

## Step 2: preview (M)

- Click a sequence to hear it with its own instrument at the session's tempo; while the session plays, it starts on the next bar, so it is heard in context.
- It is rendered on a thread of its own, through its voice, in a session of its own at that tempo, so it sounds as a drop would, and the renders are kept (the last eight) so going back to a sequence doesn't render it again. Up and Down move through the list while it plays.
- **Builds on:** `OfflineRender` (fresh plug-ins and `mixSession`), and a player added to the audio device beside the session, as the Sound Library's preview is.
- **Tests:** frames to the next bar; a render's length at two tempos and that it sounds; the player renders, plays, stops, plays again from what was kept, and says why a sequence with no instrument is silent.
- **Later:** transposing to the session's key, once a session has a key (step 6's "fits the session" filter needs it too).

## Step 2b: a master track (M) — done

Built as planned. The master is a bus flagged as the master, kept last. Its latency is made up by reading what plays into it ahead, as for a bus. Every session in the app gets one in all its undo steps. A session saved before it opens with the rack's effects on its master.

The session has no master track today: no master volume or its automation, and the master bus effects are the editor's rack, kept outside the session. A DAW lists the master with the tracks for automation, so it becomes one:

- **A master track** at the bottom of the track list and on the right of the mixer: its volume (with a fader and meter), its envelope, and its automation modes like any track.
- **The master bus effects move into the session:** its effects chain is the master track's, saved in the session file and in projects, with each effect's parameters automatable as a track's are. In the editor the rack stays the editor's own.
- Mix Down goes through the master's effects and volume, as playback does. A session from before it opens with the rack's effects on its master and the volume at 0 dB.
- **Builds on:** the track's automation (envelopes, modes, `TrackEffects` with an id of its own for the master) and the mixer strips.
- **Tests:** the master's volume envelope and an automated effect parameter play and mix down sample for sample alike; an older session opens unchanged in sound; undo and the session file keep the master.

## Step 3: the Sequencer window (L)

- Its own workspace: the library on the left, the Piano Roll, drum grid and controller lanes in the middle editing the sequence file itself, and the voice (instrument and effects) on the right.
- Its own transport: tempo (its own, or the session's), loop and metronome.
- New, Save, Save As, Duplicate, and **Make Variation**, which saves a copy linked to the original ("variation of").
- **Tests:** an edit saved and opened again is exact; a variation names its original; the window's tempo plays the notes on its beats.

## Step 4: the writing tools (L)

- **Step sequencer** for drums: per-step velocity, probability and swing, on the drum grid.
- **Arpeggiator:** an arp built from a held or drawn chord, written as notes (up, down, up and down, random, by octaves, a rate in the grid).
- **Chords:** a progression written in Roman numerals in a key and scale.
- **Scale lock**, **humanize** (timing and velocity, by amounts), and **Make Variation** with changes (velocity, notes within the scale, density).
- **Tests:** each tool's notes, checked against what it should write, and seeded where random.

## Step 5: the idea board (L)

- A grid of parts (rows) and scenes (columns) of sequences, each cell launched on the bar, to hear what works together.
- **Send to Session** lays the scenes out as arranger sections, with the tracks and clips filled in.
- **Tests:** launches land on bars; Send to Session makes the tracks, clips and sections exactly as the board holds them, as one undo step.

## Step 6: kits, MIDI files and packs (M)

- **Kits** (`.tqkit`): a set of sequences meant to play together, dropped as a track for each under a folder track.
- **MIDI files:** export a sequence as a `.mid`, and import `.mid` files into the library, choosing a voice for them.
- **Packs:** a folder of sequences zipped to share, and opened into the library.
- Ratings, favourites, sorting and the **fits the session** filter (key and scale) in the Sequence Library panel, and the library's folder chosen in Settings.
- **Tests:** a kit dropped makes every part; a `.mid` exported and imported again keeps the notes; a pack opens into the right folders.

## Step 7: Ask Torq writes and reviews sequences (L)

It can come any time after step 3, since it works in the Sequencer window.

- **Writing:** with the Sequencer open and an instrument loaded, Ask Torq writes a sequence on request ("two bars of broken-beat hats in A minor", "a pad progression, i–VI–III–VII, a bar each"). It writes notes and lanes in the sequence's own format (ticks, pitches, velocities). They go into the editor as one undo step, to hear and keep or undo. It knows the voice's instrument and what it is for (drums, a pad, a bass), the role and key, and, when one is open, the session's tempo and key.
- **Reviewing:** asked to look at a sequence, it reads the notes and gives a short review against the role:
  - rhythm and groove, and timing that is too stiff or too loose;
  - the spread of velocities;
  - the voicing and range, notes that clash with the key, and parts that crowd each other on a kit;
  - repetition over the loop.
  
  It then suggests changes, each one to apply by itself or all together as one undo step:
  - edits to the notes;
  - controller lanes, such as the mod wheel or a filter sweep;
  - automation of the instrument's parameters;
  - effects for the voice, such as a delay on an arp or a sidechain-style pump on a pad.
- **Builds on:** Ask Torq's tools today (`get_session`, the chain tools such as `apply_chain`, the mock server for tests) and the endpoints in Settings > AI, which are local OpenAI-compatible servers. New tools: `get_sequence`, `write_sequence` and `suggest_for_sequence`. Each takes and returns the sequence as JSON in ticks.
- **Tests:** against the mock server: a written sequence lands in the editor as one undo step and is exact; a review's suggestions apply one at a time and all at once, and each undoes; notes outside the key are named. The quality of the music it writes depends on the model. The guide says so, and the AI eval set gains a few sequence prompts.

## Not in 0.20

- Audio loops in the sequence library: they belong in the Sound Library.
- Sharing online.
- MIDI effects plug-ins in a voice's chain.
