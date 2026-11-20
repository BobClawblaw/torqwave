# Multitrack sessions

[Guide](README.md) › Multitrack sessions

A session lays audio files out as clips on tracks, mixes them through buses and effects, and writes the result to a file. It doesn't change the audio files it uses: clips point to them.

Switch between the editor and the session with the **Editor | Multitrack** switch at the top left, or **View > Multitrack Session** (Cmd-4). The **Multitrack** workspace (Window > Workspace > Multitrack) puts the mixer underneath and the clip panels beside the lanes.

![A multitrack session with three audio tracks, an instrument track, a reverb bus and the mixer](../screenshots/multitrack.png)

## Start a session

- **File > New Session** (Shift-Cmd-N) starts an empty one, and **File > Open Session...** (Shift-Cmd-O) opens a saved `.tqsession`.
- **Bring audio in:**
  - **Multitrack > Import Audio...** (Cmd-I) puts each file on a new track at the cursor.
  - Drag files from Finder, from the Files panel, or from the Sound Library onto the lanes.
  - Dropping below the last track makes a new track.
- **Bring MIDI in:** **Multitrack > Import MIDI File...** adds a Standard MIDI File's tracks as note clips. See [Instruments and MIDI](07-instruments-and-midi.md).

**Right-click** works almost everywhere in the multitrack view:

- on a lane, below the tracks, on the ruler, or in the track list;
- on a track's name.

The menu offers what you can do at that spot: Add Track, Add Instrument Track, Add Bus, Paste Here, Select All Clips, and more.

## Tracks

The track list on the left holds a header for each track. Drag its right edge to make it wider.

| Control | What it does |
|---|---|
| Name | Double-click to rename it; right-click for the track's menu |
| **FX** | The track's effects, the same way as the [effects rack](04-effects-and-the-rack.md#the-effects-rack) |
| **R** | Arms the track for recording |
| **M**, **S** | Mute, and Solo (play only the soloed tracks) |
| Volume, pan | Drag them; they show a value and follow an envelope when one is drawn |
| **In** | The input the track records from |
| **Sends** | Sends to buses, the output and ducking (see below) |
| **A** | The track's envelopes: volume, pan, and the parameters of its effects. It is lit when the track has automation |

The **Multitrack > Tracks** menu adds tracks, buses and folders, and also:

- duplicates, deletes and moves tracks, and hides them (they still play);
- saves a track as a template and adds tracks from templates;
- clears every solo and mute at once.

Right-click a track's header, or an empty part of a lane, for the same additions, and on a header for what applies to that track: its folder, group, colour, note, hiding and deleting it.

Drag a track's header to move the track. Where it lands also decides its folder: dropped on a folder's header, it goes into that folder; dropped on a track, it takes that track's folder, or none. Drop it on its own folder's header to take it out, just above the folder.

**Up** and **Down** select the track above or below.

## Clips

- **Move a clip:** drag it, onto another track if you like.
- **Copy a clip:** Option-drag it. The clip stays where it was and a copy goes where you drop it; with several clips selected, they are all copied. The cursor shows a plus while Option is held over a clip, and you can press Option during the drag too. The copies are left selected, and one undo removes them.
- **Trim it:** drag its left or right edge.
- **Fade it:** drag the small squares at its top corners.
- **Crossfade two clips:** with **Crossfade Overlaps** on (X), a clip dragged over part of another crossfades with it.
- **Layer clips:** where clips overlap on a track you hear both, mixed. Turn on **Layer Overlapping Clips** (in Multitrack > Editing and Recording) and you hear only the clip on top, as the lanes show it; over a crossfade you still hear both. Right-click a clip and choose **Bring to Front** or **Send to Back** to change which is on top. The setting is kept with the session.
- **Line things up:** **Snapping** (S) lines dragged clips, edges, fades and points up with clip edges, the cursor and the ruler.

Select several clips with Shift-click, or drag across empty lane space with the Select tool. They then move, nudge, copy, mute or delete together.

### Closing the gap when you delete (ripple)

**Multitrack > Editing and Recording** has three Ripple choices that set what deleting or cutting clips does to the time they played over:

| Setting | What happens |
|---|---|
| No Ripple | The gap stays where the clip was |
| Ripple Track | The later clips on the same track move back to close it, with that track's envelopes; other tracks and markers stay |
| Ripple All Tracks | The time comes out of the whole session: every track's later clips, the envelopes and the markers move back together, so they stay in sync |

Ripple All Tracks takes out only time that is empty on every track: if another track still plays over it, nothing moves. Locked clips stay where they are. The same choice is in **Settings > Multitrack**. Torqwave starts with No Ripple, and keeps the choice you make.

The setting covers a time range too. Draw one with the Range tool (6), or across the marker lane under the ruler, and **Delete** or **Cut** (Cmd-X) acts on the range instead of the selected clips, on each track with something in it:

| Setting | Delete or Cut on a time range |
|---|---|
| No Ripple | The range is cleared and the time stays empty; clips across its edges are cut back |
| Ripple Track | The range is cleared and the later clips on those tracks move back to close it; other tracks and markers stay |
| Ripple All Tracks | The time comes out of the whole session, as Delete Time does |

Cut copies what was in the range, so **Paste** (Cmd-V) puts those clips at the cursor. **Copy** (Cmd-C) copies them without taking anything out. The Edit menu says which it will do: Delete Range or Delete Clip. A click on a clip, a point or an empty lane turns Delete, Cut and Copy back to the selected clips.

### The tools

The toolbar under the lanes holds the tools. Each has a number key:

| Tool | Key | What a click or drag does |
|---|---|---|
| Select | 1 | Selects and moves clips; a drag across empty lane space selects the clips it crosses |
| Draw | 2 | On an instrument track, drags out a note clip |
| Split | 3 | Cuts a clip where you click; with Shift, every track's clips there |
| Stretch | 4 | Drags a clip's edge to make it longer or shorter, pitch kept (Option-drag does this with the Select tool) |
| Glue | 5 | Joins a note clip with the next one |
| Range | 6 | Selects a stretch of time across every track, leaving the clips as they are |
| Mute | 7 | Silences a clip, or lets it be heard again |

Beside the tools are the tempo (click it to set it) and the **Bars** switch, which counts the ruler in bars and beats.

### Clip commands

**Multitrack > Clips** has:

- Split Clip (Cmd-K: every selected clip the cursor crosses), Split All Tracks (Shift-Cmd-K: every clip under the cursor, on every track), Duplicate Clip (Cmd-D) and Repeat Clip to Range End;
- Reverse Clip, Stretch Clip... and Shift Clip Pitch...;
- Lock Clip, Mute Clip, and Group and Ungroup Clips.

**Clip Gain Lines** shows a gain line on each clip:

- Click the line to add a point.
- Drag a point to move it.
- Click a point to select it (Shift-click adds or removes more), then press Delete or Backspace to remove them.
- Option-click a point to remove it. With Option held, the pointer over a point turns into a dot with a red minus; over the line, Option-click adds nothing.

**Clip Properties** (Shift-Cmd-I) shows the selected clip's name, start, length, gain and fades, for reading and typing.

**Multitrack > Arrange** has:

- **Nudge Earlier and Nudge Later** (comma and full stop) move the selected clips to the grid point before or after.
- **Move Clips to Cursor** moves them together, so the earliest one starts at the cursor.
- **Trim Start to Cursor and Trim End to Cursor** (Option-[ and Option-]) cut the selected clips at the cursor.
- **Close Gaps** moves a track's clips back until each starts where the one before ends.
- **Quantize Clips to Grid** moves each selected clip to start on the nearest grid line.

## Editing time across every track

Select a time range with the Range tool (6), or drag along the marker lane under the ruler. Then **Multitrack > Time Range** can:

| Command | What it does |
|---|---|
| Clear Range | Empties the range on every track and leaves the time |
| Delete Time | Takes the range out and moves everything after it back |
| Insert Time | Adds as much empty time as the range is long |
| Cut Time, Copy Time, Paste Time | Move a whole section (clips, envelopes and markers) somewhere else |
| Duplicate Time | Repeats the range straight after it (Option-Cmd-D) |

This is how you take out a verse, or repeat a chorus, in every track at once.

## Buses, sends and routing

A **bus** mixes what is sent to it, through its own effects. A reverb bus is the classic use: send several tracks to one reverb, instead of putting a reverb on each.

1. **Multitrack > Tracks > Add Bus** (Shift-Cmd-B), then put the effect on the bus with its **FX** button.
2. On each track, click **Sends**:
   - Turn on the send to that bus and set its level.
   - **Pre** sends before the track's volume, so the send doesn't follow the fader.
   - **New Bus** makes a bus from there.

The same **Sends** panel holds the rest of a track's routing:

- **Output:** a track or bus can play into the mix, into a bus in its place, or out of another pair of the interface's outputs. A pair of outputs plays past the rack and is left out of mixdowns.
- **Duck under:** the track comes down while another track is heard. Use it to keep music under a voice.

**Sidechain:** a plug-in with a sidechain input can take a key track, chosen in the track's **FX** menu. For example, a compressor on the bass can be keyed from the kick.
- **Folders:** **Add Folder** puts the selected track into a new folder (from a track's right-click menu, that track). A folder's mute and solo act on all its tracks, and it can be collapsed. A track leaves its folder by **Take Out of Folder** (in the Tracks menu and its right-click menu) or by dragging, as above. Deleting a folder keeps its tracks, out of it; its right-click menu can also delete them with it. **Export Folder Stems...** writes each folder as one file.

## The mixer

**Window > Mixer** (Shift-Cmd-M) shows a strip for each track and bus, with:

- pan;
- mute, polarity and solo;
- the fader and the meter.

The master meter is on the right.

## Automation

An envelope makes volume, pan, an effect parameter or, on an instrument track, an instrument parameter change over time.

- **See it:** a track with automation shows its envelope over its lane: volume first, else pan, else the first effect parameter that has one. To see another, or none, click the track's **A** button and choose it, or **Hide Envelope**. A track you hide stays hidden.
- **Draw it:** click the **A** button and choose the envelope to show: volume, pan, a parameter of the track's instrument (**Show <instrument> Envelope**, on an instrument track), or a parameter of one of its effects (**Show 1. <effect> Envelope** and on, numbered as in the FX menu). Click the line to add a point and drag points to shape it.
- **Remove points:** click a point to select it (Shift-click adds more), then press Delete or Backspace. Or Option-click a point: with Option held, the pointer over a point shows a red minus, and an Option-click on the line adds nothing.
- **Write it while playing:** turn on **Write Automation** (W) and play. Moving the track's volume or pan, or a control in its instrument's or an effect's own window, then writes into that envelope, and the envelope shows as it is written, unless you hid it.

![The Keys track's volume envelope, shown with its A button: the line drops for the verse and rises into the chorus](../screenshots/guide-automation.png)

*The Keys track's volume envelope, from -4 dB down to -11 dB for the verse and up to -2 dB for the chorus. The A button is lit because the track has automation.*

A parameter that has an envelope is marked "(automated)" in the menu.

## Tempo and bars

- **Set the tempo:**
  - **Multitrack > Tempo > Tempo...** sets the tempo and the beats in a bar.
  - **Tap Tempo** (T) sets it from your taps.
  - **Tempo Change at Cursor...** changes it from the bar the cursor is in.
- **Count in bars:** **Bars and Beats** (B; in **Multitrack > Tempo** and **View > Time Format**) counts the ruler in bars and beats. With snapping on, a time range and the loop then go onto the grid as you drag: the nearest line of the ruler's grid (beats, or bars when zoomed out) however far the mouse is, or a clip edge or marker close by. Clips still snap only near a line. **Time with Bars** shows the time underneath as well. The session keeps the choice: it opens in bars next time.
- **The track inspector:** a panel (**Window > Track Inspector**, on the left in the Multitrack workspace) for the selected track, as Cubase's inspector: its name (type a new one), what kind of track it is, its volume, pan, mute and solo, its output (the master or a bus), its instrument, its inserts and its sends with their levels and pre-fader switches. Each change is an undo step and follows the track's group. Click the instrument or an insert to open its window; add and order effects in the Effects Rack.
- **The info line:** a strip over the tracks shows the selected clip's name, start, length, gain, fades and note (a note clip's name, start, length and loop), as Cubase's info line does. Type into a field and press Return to change it, as one undo step. Counting bars (Bars and Beats), the start is a place in bars (`5.2.1.0`: bar, beat, sixteenth, tick) and the length and loop are bars long (`2.1.0.0`: two bars and a beat), typed the same way; the fades stay in seconds. **Window > Info Line** shows or hides it; the **Clip Properties** panel has the same fields a row each.
- **The grid:** the toolbar under the tracks has **Snap** (snapping on or off) and a **Grid** box, also in **Multitrack > Grid**: Auto follows the zoom (beats, or bars zoomed out); Bar, 1/4, 1/8, 1/16, 1/32, 1/8 Triplet and 1/16 Triplet hold at any zoom, as Cubase's grid does. It applies in bars and beats, and the lanes draw it (bar lines strongest) while its lines are far enough apart to see. The choice is kept for next time.
- **Bars in the transport:** in bars and beats the clock reads bar, beat, sixteenth and ticks (120 to a sixteenth), e.g. `5.2.3.60`. **Tap**, beside the tempo in the toolbar, sets the tempo from your taps (Tap Tempo, T).
- **Loop a stretch of time:** a session has a loop of its own, as a cycle between locators, apart from what is selected.
  - Drag along the top half of the time ruler to draw it, as in Cubase (the lower half scrubs). In bars and beats its ends go onto the grid wherever you let go. It shows as a band.
  - Drag inside the band to move it, or at an end to size it.
  - Or type it: the two locator fields in the toolbar show its start and end (in bars while counting bars, else in the time format). Double-click one and type a place, such as `9` or `9.1.1.0`. With no loop yet, the other end goes a bar away.
  - The loop is lit down every lane as well as in the ruler, so what will go round shows on every track.
  - **Transport > Loop to Selection** (Option-P) makes it the time range or the selected clips; **Clear Loop** removes it. Right-click the strip for both.
  - **Loop Playback** (Cmd-L, or the loop button in the transport) goes round it. The band is brighter while it is on. Playing from before the loop runs into it, then goes round. Playing from after it plays on.
  - The session keeps the loop. Setting it is not an undo step, and an undo leaves it where it is.
- **Choose how each track follows the tempo:** right-click a track and choose **Musical Timebase**.
  - On a musical track, clips keep their place in bars and beats when the tempo changes.
  - On a track in time, clips stay where they are.
- **Hear the beat:** the **Metronome** (C) clicks on each beat while the session plays.

## Getting the mix out

**Multitrack > Render and Export** has:

| Command | What it writes |
|---|---|
| Mix Down... (Shift-Cmd-E) | The session, or its time range, to one file. It can be delivered to a [loudness spec](05-loudness-and-mastering.md#meet-delivery-spec) |
| Export Stems... | One file for each track, through its own effects |
| Export Folder Stems... | One file for each folder |
| Export Regions... | One mixed file for each region |
| Bounce Track | Renders a track through its effects onto a new track, and mutes the original |
| Freeze Track | Renders a track's effects so they stop using the processor while you work |

A session can also be mixed in 5.1 or 7.1 (**Multitrack > Surround**). See [Video and surround](09-video-and-surround.md#surround-sessions).

## Saving

- In the multitrack view, **File > Save** becomes **Save Session** (Cmd-S), and Save As becomes **Save Session As...**. A session is saved as a `.tqsession` file. It points to the audio files, so keep them where they are.
- **File > Save a Copy with Audio...** writes a copy of the session with every audio file it uses in a folder beside it, to move it or hand it on.
- **A project** keeps the session together with the files open in the editor.
- **After a crash,** Torqwave offers to recover the session the next time it starts.

Next: [Instruments and MIDI](07-instruments-and-midi.md).
