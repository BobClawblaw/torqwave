# Plan 0.21: Torqwave Sampler, a built-in sampler

Torqwave has three instruments of its own (Torqwave Synth, Basic Drums and StringSwarm) and none of them plays a recording. Torqwave Synth's Sampled table takes a sound's tone, not the sound: the pitch comes from the keys and the timing is gone. A feature comparison made for 0.20.0 found that nine of the ten DAWs compared ship a sampler, and this is the widest gap in it.

This plan adds **Torqwave Sampler**, a fourth built-in instrument:

- **An engine** that plays samples across the keys in zones, with loops, envelopes, a filter and an LFO.
- **Slices:** one sample cut into pieces, each on its own key, so a break or a field recording becomes a kit that the drum grid and Remap Notes can drive.
- **Sound straight from the rest of Torqwave:** the editor's selection, a session clip, the Files panel and the Sound Library. A program that is both an editor and a DAW can do this in one step, where others need an export and an import.
- **A window** like the other instruments', and the same wiring into sessions, presets, Audition and Ask Torq.

Each piece below has:

- what it is;
- what it builds on in the code today;
- its size: **S** is days, **M** about a week, **L** more;
- how it is tested.

Each step lands as its own commit with its tests, guide pages and a changelog entry that says what was not tried by hand. Problems found by use go into the [fix log](DOGFOODING.md) with an issue, as each is fixed.

## Decided

The user decided these on 2026-11-23, before any code:

| Question | Decision |
|---|---|
| The name | **Torqwave Sampler** (plug-in ID `torqwave:sampler`) |
| Factory content | A small set, rendered from Torqwave's own three instruments, plus sliced kits. The guide says plainly that it is small. Nothing comes from anyone else's samples or presets (the clean-room rule) |
| Samples in presets | A preset names its sample files by path. No audio is embedded in a preset. Projects copy the files in; a file that has moved is asked for |
| SFZ import | Not in 0.21.0. It can follow in 0.21.x |

## Not in 0.21.0

- **Disk streaming.** Samples load whole into memory as 32-bit float. One sample can be up to 10 minutes long, and one sampler holds up to 1 GB of samples (about 46 minutes of 48 kHz stereo). A sample over either limit is refused, with the reason. Two samplers playing the same file hold it once.
- **Importing sample libraries:** SFZ, SF2 or any other format.
- **Round robins.**
- **Time-stretch per key.** A sample played higher is shorter, as on any sampler that resamples.
- **Several outputs.** It has one stereo output.
- **MPE.**
- **Dragging a session clip onto the window.** Clips don't drag out of the multitrack view today. A clip reaches the sampler by commands in its menu instead (step 6).

## What the code gives us to build on

- **Built-in instruments** (`BuiltInEffects.h`, `BuiltInEffect`): a base class with hosted parameters, a state written as XML (`addToState`, `readFromState`), and presets (`getPresetValues`, `setPresetValues`). A new instrument needs an ID, a description, a line in `createPluginInstance` and an entry in `getDescriptions()`. `Instrument::known` and the Add Instrument Track menu then list it without more code.
- **StringSwarm** (`SwarmGuitar.h/.cpp`, `SwarmGuitarEditor.cpp`, `SwarmGuitarPresets.cpp`, `Tests/SwarmGuitarTests.cpp`) is the smallest full example: voices that fade when stolen or on All Sound Off, pitch bend, volume, expression and the sustain pedal, the tempo read from the play head, and tests that render at several block sizes and twice.
- **Basic Drums** (`DrumMachine.h`): an engine that knows nothing of plug-ins, so the window can render a sound to draw it and the tests can play it bare; a voice for each note; choke groups; pads clicked in the window handed to the audio thread.
- **Torqwave Synth** (`WaveSynth.h`): controls taken afresh every 16 samples of the song's clock and ramped between, which is how it renders the same at any block size; a sound swapped in from the message thread while voices play (`setSampledTable`: a lock the audio thread only tries, and the old sound kept until the voices have left it); a preset that carries text beside its values (`getPresetTexts`).
- **The presets row** (`EffectPresets::Bar`) with previous and next, and its `carryTexts`. The instrument's window (`MainComponent::PluginEditorWindow`) finds that row in any built-in's editor and adds Audition, Loop, Stop, the pattern menu and the playing keys (`InstrumentKeys`). `InstrumentKeys::kindFor` chooses what Audition plays for each instrument.
- **Sounds dropped on an instrument's window** (`SoundDrops.h`): the Files panel's rows give a sound to Decrypt's scope and to the synth's oscillators. Finder files and the Sound Library arrive as file drops.
- **Pitch:** `WaveTables::findPitch` (autocorrelation, 30 Hz to 2 kHz, none below a confidence), which Decrypt uses to find a recording's note.
- **Crossfade Loop** (`AudioEdits::crossfadeLoop`): an equal-power crossfade of a loop's end into the audio before its start. It does not find the loop; the user selects it. `AudioEdits::zeroCrossingNear` finds zero crossings.
- **Hits:** `TempoFollow::transients` (attacks found by a rise in the highs), which Slice at Transients and Warp Markers at Transients use. `AudioEdits::soundsBetween` finds the sounds between silences.
- **Markers and regions** (`Markers.h`): read from WAV and AIFF files by `AudioFileIO::load`.
- **Session and project files:** a session names each clip's audio by a path relative to its folder and by its full path, and tries them in that order (`SessionFile`). **Save a Copy with Audio** (`SessionCollect`) and a project's Files folder (`Project`) copy audio in.
- **Drum maps** (`DrumSound`, `DrumMaps`, the Piano Roll's drum grid): a track's own rows, each with a note and a name, and Remap Notes.
- **Ask Torq** (`AiSession.cpp`): session tools tested against a mock server, `table_from_clip` among them.

**Four things the sampler needs that are not there yet.** Each is built in the step named:

- **No reader of a WAV file's sampler chunk.** Torqwave ignores the root note and loop that a WAV's `smpl` chunk holds. Step 4 reads them.
- **No finder of loop points.** Step 4 adds one and uses Crossfade Loop's crossfade.
- **No interpolator for playing a sound at any speed, voice by voice.** The sample-rate converter works on whole files at one ratio, and the scrubber's four-point curve is not clean enough for an instrument. Step 1 adds one.
- **No relinking of missing files.** A session with missing audio reports it and leaves the clips out. Sessions and projects also know nothing of files named inside an instrument's state. Step 4 covers the sampler's own files. Relinking a session's clips is not part of this plan.

## Step 1: the engine, one sample across the keys (M to L, done)

Built in `Source/Sampler.h/.cpp` as planned. The windowed sinc is 48 points, and at its own pitch and rate a sample plays frame for frame (both settled in step 3).

A new instrument, **Torqwave Sampler** (`Source/Sampler.h/.cpp`, ID `torqwave:sampler`), listed after StringSwarm. As with Basic Drums, the engine knows nothing of plug-ins.

- **Playing a sample:** at the played note's pitch by resampling, from the sample's own rate, so no file is converted when it loads. Mono and stereo samples.
- **Interpolation:** a windowed sinc, with a narrower band when a sample is played above its pitch, so playing up does not alias. The target is images and aliases at least 80 dB under a full-scale tone from two octaves down to two octaves up. The changelog gives what was measured.
- **Voices:** 32 at once, the oldest taken over. A stolen voice and All Sound Off fade over 4 ms, as Torqwave Synth's do. Nothing stops dead: a sample that ends, or starts away from zero, is faded over a fraction of a millisecond.
- **Amp envelope:** Attack, Decay, Sustain, Release, and how much velocity sets the level.
- **Filter:** off, low-pass (12 and 24 dB), band-pass or high-pass, with Cutoff, Resonance, key tracking and an envelope of its own (Attack, Decay, Sustain, Release and an amount), and velocity to cutoff.
- **LFO:** a rate in Hz or in note values at the session's tempo, a shape, a fade-in, and amounts to pitch, filter, level and pan.
- **Voice settings:** Level, Pan, Transpose, Fine tune, the pitch bend range, Poly or Mono, and Glide.
- **MIDI:** notes with velocity, pitch bend, mod wheel (to the LFO's depth), volume (7), pan (10), expression (11), the sustain pedal (64), All Notes Off and All Sound Off.
- **Hosted parameters:** all of the above, so automation, presets and sessions find them. Zones are not parameters: they are kept in the state.
- **The same every time:** controls are taken every 16 samples of the song's clock, counted from each note's start, so a part renders the same at any block size and the same twice.

**Builds on:** `BuiltInEffect`, StringSwarm's voice handling and controllers, Torqwave Synth's control interval and its way of swapping a sound under playing voices.

**Tests** (`Tests/SamplerTests.cpp`, run alone with `--sampler`):

- A sine sample played at its root is that sine. Played an octave up it is twice the frequency and half as long.
- The interpolator's aliases and images across four octaves, against the target.
- The envelope's times, velocity to level, the filter's response and its envelope, the LFO's rate free and at a tempo.
- Pitch bend, volume, expression, pan and the sustain pedal.
- A thirty-third note takes the oldest voice without a click. All Sound Off fades in 4 ms. No sample-to-sample step above a threshold anywhere in a busy render.
- Bit-identical at block sizes from 1 to 4,096 and on a second run.
- Its state saves and restores exactly. It is created by its description, and the count of built-ins goes from 21 to 22.

## Step 2: zones, layers and loops (M, done)

Built as planned. One change: a loop's crossfade is a zone's setting, not a hosted parameter, and a change to it under a held note is taken up at the next place clear of the loop's turn.

- **Zones:** up to 128 in one sampler. Each has a sample, a root note and fine tune, a key range, a velocity range, a level and a pan. Zones that overlap play together, so layers and velocity switches are zones.
- **Start and end** within the sample, and **Reverse**.
- **One-shot or held:** one-shot plays to the end whatever the note's length (a drum hit); held follows the envelope.
- **Loop:** off, forward, or forward and back, between a loop start and a loop end, while the key is down or through the release as well.
- **Loop crossfade:** a length in milliseconds. The engine builds the crossfaded turn in a buffer of its own, with the same equal-power fade as Crossfade Loop, so the file is never changed and the length can be turned while it plays.
- **Choke:** a zone can cut the other zones of its group, as hats do.

**Builds on:** step 1; `AudioEdits::crossfadeLoop`'s crossfade; Basic Drums' choke groups.

**Tests:** each key and velocity plays the zones that cover it, and only those. A looped tone stays continuous for ten seconds: no step at the turn, with and without a crossfade, forward and back-and-forth. A reversed sample is the sample backwards. A one-shot rings past its note-off, and a held one releases. A choked zone fades in 4 ms. Block sizes and second runs as in step 1. 128 zones save and restore.

## Step 3: slices (M, done)

Built as planned. The slices start on C2 (key 36: Torqwave names middle C C4), which the plan first called C1. The drum map is made by the engine and put on a track by Slice to Sampler Track (step 6); there is no button for it in the window.

The second way to play: **Slices** in place of **Keys**. One sample is cut into pieces and each piece plays on its own key, upward from C2 (36, where a kick sits), up to 64 slices.

- **Where the cuts come from:**
  - the file's **markers** (a slice between each pair);
  - its **regions** (a slice each);
  - **detected hits**, with a sensitivity setting;
  - **equal parts:** 2, 4, 8, 16, 32 or 64.
- **Each slice** is a zone, one-shot to start with, with its own level, pan, tuning and reverse. A slice's end is faded so it does not click. **Cut each other** makes every slice stop the last one, as playing a break by hand needs.
- **Cuts can be moved, added and removed** by hand (in the window, step 5). Cuts from hits land on the nearest zero crossing.
- **Notes from Slices:** writes a note clip that plays the slices in order at the times they have in the sample, so the break plays as it was, and can then be rearranged in the drum grid.
- **Names for the drum grid:** the sampler gives its track a drum map with a row for each slice (the marker's or region's name, or "Slice 1" on), so the drum grid shows the slices. Each row can be told which General MIDI drum it stands for, and then Remap Notes and dropped drum presets land on the slices.

**Builds on:** `TempoFollow::transients`, `MarkerEdits`, `AudioEdits::zeroCrossingNear`, `DrumSound` and the drum grid.

**Tests:** a file with eight markers gives the slices between them, on keys from 36. Regions give one slice each. A rendered drum pattern with hits at known times is cut within a few milliseconds of each (the changelog gives the measured error). Equal parts are equal. Notes from Slices, played back through the sampler at the sample's tempo, rebuilds the sample to within the fades. The drum map names the slices, and Remap Notes moves General MIDI notes onto the slices that stand for them.

## Step 4: files, pitch and loops found for you (M, done)

Built as planned, with one narrowing: a project copies the samples into its Files folder only when its session is kept there (one never saved on its own); a session with a file of its own keeps naming them where they are, and Save a Copy with Audio is the way to gather them. A sound with no clear pitch gets C4 (key 60).

- **Loading:** through `AudioFileIO::load`, so every format Torqwave opens, with its markers. A sample is loaded once and shared by every sampler that plays that file. The limits are those under "Not in 0.21.0", and a refused file says why.
- **The WAV sampler chunk:** when a file has one, its root note and loop are taken from it.
- **Root pitch found:** otherwise the pitch is measured half a second in, after the attack (as Decrypt does), and sets the root note and fine tune. A sound without a clear pitch gets C4 and says so.
- **Find Loop:** in the held part of a note, looks for a start and an end a whole number of the sound's periods apart where the two places are most alike, moves both to zero crossings, and sets a crossfade. It is a suggestion: the handles are there to change it.
- **Where the files are kept in the state:** each sample by a path relative to the session's folder and by its full path, as clips are, and found in that order. A factory sample is named relative to the app's own samples.
- **Sounds that are not files** (an editor selection, an unsaved file) are written as 32-bit float WAV files into **Music > Torqwave > Samples**, beside the Sequence Library's folder, and named from where they came.
- **Copying into a project:** saving a project copies the sampler's files into the project's Files folder, and **Save a Copy with Audio** copies them into the copy's Audio folder, and the state then names the copies.
- **Missing files:** a sample that is not found leaves its zone silent and the rest playing. The session's "Missing audio" message lists it, and the window shows it with **Locate...**. A file located in a folder is tried for the other missing samples too.

**Builds on:** `AudioFileIO::load`, `WaveTables::findPitch`, `AudioEdits::crossfadeLoop` and `zeroCrossingNear`, `SessionFile`'s paths, `SessionCollect`, `Project::filesFolder`.

**Tests:** a WAV written with a sampler chunk loads with its root and loop. Tones at known pitches, low and high, are found to within a few cents, and noise is given C4. Find Loop on a held synth note gives a loop whose turn can't be seen in the render (the step at the turn is measured and given in the changelog). A session with a sampler survives its folder being moved. A project and a copy with audio hold the samples and play with the originals deleted. A deleted sample is reported, its zone is silent, and locating it brings it back. A file over a limit is refused with the reason. Two samplers on one file hold one copy.

Fresh copies of an instrument are made for mixdown, freeze and bounce, so a sampler's state loads its samples before it returns. With the live instrument holding them, a copy takes them from the shared store.

## Step 5: the window (L, done)

Built in `Source/SamplerEditor.h/.cpp` as planned, 940 by 746.

Laid out as the sound flows, with the same knobs and panels as the other instruments (`SoundControls`).

- **The presets row** along the top. Audition, Loop, Stop, the pattern menu, previous and next, and the playing keys come with it, as for every built-in instrument.
- **The waveform** of the selected zone: zoom and scroll, handles for start, end, loop start, loop end and the crossfade, the slices' cuts in Slices mode (drag one, double-click to add, Option-click to remove), and a line where each sounding voice is playing.
- **The key map:** the zones drawn over the keys, each with its root marked. Drag a zone's edges to set its key range, and drag up and down for its velocity range. Click a zone to select it. In Slices mode, a pad for each slice that lights as it plays.
- **The zone's settings:** root, tune, level, pan, reverse, one-shot or held, loop mode, crossfade, choke group; **Find Pitch** and **Find Loop**.
- **The sound's settings:** amp envelope, filter and its envelope, LFO, voice settings.
- **Keys / Slices** and, in Slices, where the cuts come from.
- **Drop a sound** anywhere on it (step 6), or double-click the empty waveform to choose a file. An empty sampler says what to do.

**Builds on:** `SwarmGuitarEditor` and `DrumMachineEditor` (panels, knobs, pads, the picture of a sound), `EffectPresets::Bar`, the peaks `AudioSnapshot` keeps.

**Tests:** the window made without a screen, as the others are: its size, every knob reading its parameter in its unit, a handle dragged moving the zone's start, end and loop, a zone's edge dragged changing its range, a cut dragged, added and removed, the pads lighting. Pictures in the dark and light themes with `--sampler-window`. A preset saved from the row puts back the zones and their files.

## Step 6: getting sound in (M, done)

Built in `Source/SamplerTrack.h/.cpp` and the main window. New Sampler Track from Selection has no key of its own. A clip reaches the sampler by its menu, as the plan says; a part dragged from the Sound Library's preview arrives as a file.

- **Drops on the window:** a file from the Finder or the Sound Library (a part of a sound dragged from the library's preview too), and an open file from the Files panel, saved or not. Dropped on the waveform, the sound replaces the selected zone's sample; dropped on the key map, it becomes a new zone at the key it lands on. Several files dropped together become zones on neighbouring keys, or, when each has a clear pitch, zones around their own notes.
- **New Sampler Track from Selection** (in the editor's menus): the selection, or the whole file with none, becomes a sample; the session gets a new instrument track with Torqwave Sampler holding it, its root pitch found; and the window opens. Markers inside the selection go with it, so **Slices** has them. A saved, unchanged file is named where it is; anything else is written to the Samples folder first.
- **In a clip's menu** (multitrack view): **New Sampler Track from Clip**, which names the clip's own file with the clip's range as the zone's start and end, so nothing is copied; and **Slice to Sampler Track**, which also cuts it at its hits, and puts the Notes from Slices clip where the audio clip is, so the session plays the same as before and every hit is now a note.
- **A key** for New Sampler Track from Selection only if one is free; `docs/SHORTCUTS.md` is updated if so.

**Builds on:** `SoundDrops`, the synth's `WavetableView` drops, `MainComponent::saveSelectionAs`, `MultitrackView::addInstrumentTrack`, Slice at Transients.

**Tests:** each drop on the window, with the hooks the tests can call. The command on a selection of a tone makes a track that plays the tone at its own pitch from its root key. Slice to Sampler Track on a rendered drum loop makes a track whose mixdown matches the clip it came from to within the fades (the difference is measured and given). Each is one undo step.

## Step 7: the rest of the wiring (M, done)

Nothing had to change but Audition's choice of phrase: the sampler is an instrument like the others. The checks are in `Tests/InstrumentTrackTests.cpp`.

- **Generate from Instrument**, **freeze**, **bounce** and **Mix Down** play it as any instrument. Tested: each against playback's own render.
- **Presets and favourites:** factory presets in the presets row by category, the user's own beside them, and whole-state instrument presets (Save Instrument Preset). A preset carries its zones and the paths of its files.
- **Audition:** hits for a sliced sampler, and for the others a phrase by the preset's category (a bass line for a bass, a chord for a pad), as Torqwave Synth's patches have.
- **Tempo:** the LFO's note values follow the session's tempo, through tempo changes and while stopped, as the other instruments' synced settings do.
- **Sequences and kits:** a sequence saved with a sampler as its voice keeps the sampler's state, files named as above.
- **Automation:** its parameters draw and write as any instrument's.

**Tests:** in the style of the other instruments' (`InstrumentKeysTests`, `InstrumentTrackTests`, `EffectPreviewTests`): previous and next through the presets, the audition phrase for each kind, a frozen and a bounced sampler track bit for bit with playback, the LFO at two tempos, a sequence's voice round trip.

## Step 8: Ask Torq (M, done)

Built as planned, except that the eval set has no sampler sentences yet, and no model has been asked for a sampler.

- **`sampler_from_clip`:** makes a clip a sampler, on a new track or on a track that already has one, across the keys or sliced. It sits beside `table_from_clip`.
- **`set_sampler`:** a zone's root, ranges, start, end, loop and one-shot; the mode; where the slices come from; Find Pitch and Find Loop.
- **What the model is told:** `get_session` gives a sampler track's samples, zones and mode; the factory presets are listed with the other instruments' (`list_instrument_presets`, `set_instrument`); its parameters are reached by the tools every instrument has.
- Every change is a plan shown before it is applied and undone in one step, as Ask Torq's others are. No audio is sent to the model.

**Builds on:** `SessionTools::tableFromClip`, `AiLibrary`'s factory sounds, the mock server.

**Tests:** against the mock server: each tool's plan and its result, refused calls with their reasons (no such clip, a track without a sampler), one undo step each. `docs/AI.md` and guide page 13 list the tools. The eval set gains a few sentences.

## Step 9: factory content (M, done)

Built: eleven presets over 26 samples, 8.2 MB, in `Data/Samples`, written by `TorqwaveTests --sampler-content`.

Small, and said to be. Every sample is rendered by Torqwave from its own instruments, by a tool in the repository, so the set can be made again and nothing in it is anyone else's.

- **Played instruments:** notes of chosen Torqwave Synth patches and StringSwarm presets, sampled every few keys with loops found, as presets under their categories.
- **Sliced kits:** bars of the factory drum presets played on Basic Drums' kits, cut at their hits, each with its drum map.
- **How much** is settled when it is built, with the download's size in mind. The guide and the changelog give the count and the size in megabytes.
- The files ship in the app beside the factory sequences, compressed without loss.

**Tests:** every factory preset loads all its samples, plays every zone, peaks under 0 dB and above a floor, and has no loop that clicks. The files in the repository are the ones the tool writes.

## Last: the guide, screenshots and the changelog (done)

Done with each step. The two pictures of the window are drawn by the tests, not taken from the running app.

- **Guide page 7:** a Torqwave Sampler section (the window, Keys and Slices, loops, the limits, what is not there), the commands on page 2 (editor) and page 6 (clips), projects on page 11, Ask Torq on page 13.
- **README:** the features list and the instruments named.
- **Screenshots:** the window on a looped note and on a sliced break.
- **`docs/TESTING.md`:** the `--sampler` and `--sampler-window` flags.
- **The changelog**, with what was measured and what was not tried by hand.
- **`ROADMAP.md`:** 0.21.0's line.

## Order and size

1. The engine, one sample across the keys (M to L)
2. Zones, layers and loops (M)
3. Slices (M)
4. Files, pitch and loops found (M)
5. The window (L)
6. Getting sound in (M)
7. The rest of the wiring (M)
8. Ask Torq (M)
9. Factory content (M)
10. Guide, screenshots, changelog

All ten are built, each as its own commit, with the whole test suite passing at each. Nothing of it has been tried by hand in the running app: that is what is left before 0.21.0.

Steps 1 to 4 are in order: each needs the one before. Step 5 needs 1 to 4, and 6 needs 5. Steps 7, 8 and 9 need 6 and can go in any order.

If 0.21.0 should ship sooner, steps 1 to 7 are a whole sampler; Ask Torq's tools and the factory content could follow as 0.21.1.

## At release

- The version in `CMakeLists.txt` and the docs goes to 0.21.0.
- If the comparison (`docs/compare/`) has been merged by then: `builtin_sampler` becomes yes for Torqwave in a 0.21.0 snapshot, and the matrix and graphics are made again.
- The release notes say what the sampler is not: no streaming from disk, no library import, no round robins, and a small factory set.
