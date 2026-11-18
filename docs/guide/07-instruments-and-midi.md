# Instruments and MIDI

[Guide](README.md) › Instruments and MIDI

An **instrument track** plays a synthesizer: Torqwave's built-in Basic Synth, or any VST3 instrument (and Audio Unit on the Mac) that your [plug-in scan](04-effects-and-the-rack.md#plug-ins) found. It holds **note clips**, not audio. You put the notes in by drawing them, typing them, playing them on a MIDI keyboard, or importing a MIDI file.

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
| Import a MIDI file | **Multitrack > Import MIDI File...** adds each track of a Standard MIDI File that has notes as a note clip on a new track, at the cursor |

Right-clicking a lane offers **Add Notes Here...** and **Add Trigger Here** at that spot.

## Play it live

The selected instrument track plays whatever comes in live:

- **A MIDI keyboard.** Turn its input on in **Audio Settings...**.
- **Window > Keyboard**, the on-screen keyboard. Click its keys, or use your computer's keys. Its line at the top says which track it plays, or why nothing will sound.
- **The instrument's own window.** Open it with the instrument button, and its keys sound even while the session is stopped.
- **The Piano Roll's keyboard** (see below).

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

Notes sound as you put them in, click them or drag them. The **grid** choice divides the beat into 1, 2, 4, 8, 16 or 32 steps, or turns the grid off.

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
- **Quantize Notes...**: move the notes to the nearest line of a grid of the beat.

Shift-click selects several clips, note and audio together, to move, nudge, quantize, mute, copy, duplicate or delete at once.

## Automate the instrument

An instrument's parameters can be automated like any effect's. Click the track's **A** button and show one of its parameters' envelopes. See [Automation](06-multitrack-sessions.md#automation).

## Freeze, bounce and export

An instrument track freezes, bounces, mixes down and exports in stems like any other track. Bounce it when you want its sound as audio to edit further.

## A sound into a file

To make a sound with an instrument and put it straight into a file in the editor, use **Effects > Generate > Generate from Instrument...**:

1. Choose the instrument. Its window opens beside the dialog, to shape the sound.
2. Set the notes: played together or one after another.
3. Preview, then generate. The sound goes in at the cursor.

Next: [Recording](08-recording.md).
