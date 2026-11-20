# Instruments and MIDI

[Guide](README.md) › Instruments and MIDI

An **instrument track** plays a synthesizer: Torqwave's built-in Basic Synth, or any VST3 instrument (Audio Unit too on the Mac, and LV2 on the Mac and Linux) that your [plug-in scan](04-effects-and-the-rack.md#plug-ins) found. It holds **note clips**, not audio. You put the notes in by drawing them, typing them, playing them on a MIDI keyboard, or importing a MIDI file.

## Make an instrument track

- **Multitrack > Tracks > Add Instrument Track...**, or right-click the lanes or the track list and choose **Add Instrument Track**. Pick the instrument from the menu; the track is named after it.
- **Turn an empty audio track into an instrument track:** right-click its header and choose **Instrument Track**. Choose it again to turn it back.

The header's instrument button, which shows the instrument's name (or **Inst** when there is none), chooses, swaps or opens the instrument. It takes the place of the record button. Effects added with **FX** run after the instrument, as on any track.

New instrument tracks follow the tempo ([musical timebase](06-multitrack-sessions.md#tempo-and-bars)), so their clips keep their place in bars and beats when the tempo changes.

## Put notes in

| Way | How |
|---|---|
| Draw a clip | The **Draw** tool (2): drag along the instrument track's lane. Below the last track, the same drag makes a new track |
| Type the notes | Right-click the track's header and choose **Add Notes...**. Type note names (C4, F#3, Bb2; C4 is middle C) or MIDI numbers (60), separated by commas. Choose whether they play together or one after another, how long each is held, and whether the pattern repeats |
| A single hit | **Add Trigger** in the same menu puts a one-note clip (C4, a tenth of a second) at the cursor. Use it for a sampler or drum instrument |
| Play them in | Turn on **Record Notes** (Option-R), play, and stop: what you played becomes a note clip |
| Keep what you just played | **Capture MIDI** (Option-Shift-R, in **Multitrack > Editing and Recording**) turns the last 60 seconds of MIDI you played into a note clip, even with Record Notes off (see below) |
| Import a MIDI file | **Multitrack > Import MIDI File...** adds each track of a Standard MIDI File that has notes as a note clip on a new track, at the cursor |

Right-clicking a lane offers **Add Notes Here...** and **Add Trigger Here** at that spot.

## Play it live

The selected instrument track plays whatever comes in live:

- **A MIDI keyboard.** Turn its input on in **Audio Settings...**.
- **Window > Keyboard**, the on-screen keyboard. Click its keys, or use your computer's keys. Its line at the top says which track it plays, or why nothing will sound.
- **The instrument's own window.** Open it from the instrument button's menu (**Instrument Window**), and its keys sound even while the session is stopped.
- **The Piano Roll's keyboard** (see below).

<img src="../screenshots/guide-synth.png" width="430" alt="The Basic Synth window: Presets, Wave, Level, Attack, Decay, Sustain, Release, Cutoff and Resonance">

*The window of Basic Synth, the built-in instrument.*

## Edit the notes: the Piano Roll

Double-click a note clip, or choose **Window > Piano Roll**, to edit the clip's notes on a grid of pitch against time. A keyboard runs down the left, with middle C marked.

| To | Do |
|---|---|
| Add a note | Click empty grid. It lands on the grid line at or before the mouse, one grid step long, at the velocity the slider shows |
| Move a note | Drag it: up and down for pitch, sideways for time (snapped to the grid) |
| Change its length | Drag its right edge |
| Select notes | Click one; Shift-click adds or takes away; drag across empty grid for a box; Cmd-A for all |
| Remove notes | Option-click one, or press Delete for the selected |
| Nudge the selected notes | Up and Down move a semitone (Shift: an octave). Left and Right move a grid step (Shift: a quarter step) |
| Set velocity | Select the notes and move the velocity slider |
| Set the loop | Drag the first repeat line |
| Hear a pitch | Click a key on the keyboard at the left; drag along it to play each key you pass |
| Zoom in or out on time | Cmd-scroll or pinch, around the mouse; or **-** and **+** at the top right. **Fit** shows the whole clip |
| Move along in time | The scroll bar under the notes, a sideways scroll, or Shift-scroll |
| Make the rows taller or shorter | Option-scroll |
| Move up and down the keys | Scroll |

A clip whose pattern repeats opens on its first repeat, so a one-bar loop in a nine-bar clip fills the width; **Fit** shows all nine bars.

Notes sound as you put them in, click them or drag them. The **grid** choice divides the beat into 1, 2, 4, 8, 16 or 32 steps, or turns the grid off.

**Swing**, beside it, moves the second line of each pair later, from Straight (50 %) to 75 %. It acts on a grid of 2 or 4 steps a beat (1/8 or 1/16 notes in 4/4), for the grid lines, snapping and the drum grid's steps.

**Ch** sets the MIDI channel the clip's notes and lanes go out on, 1 to 16, for an instrument that plays a different sound on each channel.

![The Piano Roll under the multitrack view, showing the Lead clip's 12 notes, with the Grid, Velocity and Lane controls along its top](../screenshots/guide-pianoroll.png)

*The Lead clip's notes in the Piano Roll. Clip Properties, at the right, shows the clip's name, start and length.*

### The drum grid

**Drums**, at the left of the top bar, shows the clip as a step grid instead: a row for each drum sound, a column for each grid step (quarter beats with the grid off). The choice is the track's, so all its clips open that way.

| To | Do |
|---|---|
| Set a step | Click it. The note is one step long, at the slider's velocity |
| Clear a step | Click it again, or Option-click it |
| Set a step's velocity | Drag up (louder) or down (softer) from it. The bar inside the step shows the velocity |
| Hear a sound | Click its name |
| Edit the map | Right-click a name: **Edit Sound...** (its name and note), **Add Sound...**, **Remove from the Map**, **Move Up**, **Move Down**, or back to the **General MIDI Map** |

The rows come from the track's drum map, General MIDI's (notes 35 to 81) unless you change it. The map is saved with the session. Notes not in the map get rows of their own at the bottom, named by pitch; **Add to the Map...** names one.

### Controller lanes

The **Lane** choice shows a controller under the notes: the mod wheel, the pitch bend, or any CC.

- Click to put a point (snapped).
- Drag a point to move it.
- Option-click a point to remove it.

The lane's values are sent to the instrument along with the notes.

## Work with note clips

Note clips act like audio clips in the lanes. They:

- drag between tracks and in time;
- trim at their edges;
- copy, paste and duplicate (Cmd-D);
- split with the **Split** tool (3) or **Split Clip** (Cmd-K);
- stretch with the **Stretch** tool (4), which scales their notes;
- join with the next clip using the **Glue** tool (5), or **Merge with Next** in a clip's menu.

**Clip Properties** shows a note clip's name, start, length and loop.

For the selected note clips, **Multitrack > Notes** has:

- **Transpose Notes...**: by semitones.
- **Note Velocity...**: set the velocity, or scale it.
- **Quantize Notes...**: move the notes to the nearest line of a grid of the beat. **Swing** (50 to 75 %, on a 1/8 or 1/16 grid) moves the off-beat lines later. **Strength** moves each note only part of the way (100 % is all the way). **Starts and ends** puts the ends of the notes on lines too, keeping each at least a step long.

### Capture MIDI

Torqwave keeps the last 60 seconds of MIDI coming in from your keyboard, whether the session plays or not. **Capture MIDI** (Option-Shift-R) turns it into a note clip named "Captured":

- **Played while the session played:** the notes land where you played them.
- **Played while stopped:** the notes start at the cursor, at the times you played them.

The clip goes on the track you played into, or on a new instrument track. What was captured is then forgotten, so a second Capture MIDI has only what you played since.

Shift-click selects several clips, note and audio together, to move, nudge, quantize, mute, copy, duplicate or delete at once.

## Automate the instrument

An instrument's parameters can be automated like any effect's:

- **Draw it:** click the track's **A** button, open **Show <instrument> Envelope** (the instrument's name, above the numbered effects) and choose a parameter. Click its line to add points.
- **Write it while playing:** turn on **Write Automation** (W), open the instrument's window (**Inst** > **Instrument Window**) and move its controls while the session plays.

The envelope is saved with the session and plays when the session is mixed down, bounced or frozen. Parameters with an envelope are marked "(automated)" in the menu. The envelope belongs to that instrument: choosing a different instrument for the track leaves the envelope unused. See [Automation](06-multitrack-sessions.md#automation).

## Hardware synths: MIDI out

A track can play a hardware synth (or another app) instead of a plug-in. In its instrument button's menu, **MIDI Out** lists the MIDI output ports. Choose one, and the track's note clips, controller lanes and what you play live into it go out of that port. The button then shows the port's name.

- **Channel:** **Each Clip's Own** sends each clip on the channel set in the Piano Roll. **Channel 1** to **16** sends everything on one.
- **Offset...:** sends the notes earlier (a negative number of milliseconds) or later than the audio, up to 500 ms either way. Use it to line the synth up with the session when its sound comes back through your interface.

The messages leave when the audio they go with is heard, allowing for the rack and the interface's output latency. Stopping, or a loop going round, ends every note left on. To record the synth's sound, give an audio track the interface input it comes back on.

MIDI out needs the session and the interface at the same sample rate: while Torqwave converts between them, nothing goes out. A port that isn't connected when the session opens is reported. Choose it again once it is.

## MIDI from another track's plug-in

An arpeggiator, chord or other MIDI plug-in can play another track's instrument. Put the MIDI plug-in on one instrument track, as its instrument. Then, on the track whose instrument should play, choose that track under **MIDI From** in the instrument button's menu. The track plays both its own notes and what the MIDI plug-in puts out.

Keep the track with the MIDI plug-in above the one it plays. Placed below, its notes arrive one block late.

## MIDI clock out

**Multitrack > MIDI Clock Out** sends MIDI clock (24 a beat, following the tempo map) and transport to a port, for drum machines, sequencers and arpeggiators to follow:

- Song Position and Start (from the top) or Continue when the session plays.
- Stop, Song Position and Continue when the loop goes round or you jump.
- Stop when it stops.

The choice is kept with Torqwave's settings, not the session.

## Freeze, bounce and export

An instrument track freezes, bounces, mixes down and exports in stems like any other track. Bounce it when you want its sound as audio to edit further.

## A sound into a file

To make a sound with an instrument and put it straight into a file in the editor, use **Effects > Generate > Generate from Instrument...**:

1. Choose the instrument. Its window opens beside the dialog, to shape the sound.
2. Set the notes: played together or one after another.
3. Preview, then generate. The sound goes in at the cursor.

Next: [Recording](08-recording.md).
