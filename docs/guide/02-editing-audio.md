# Editing audio

[Guide](README.md) › Editing audio

This page covers editing one file in the editor: selecting, listening, cutting and pasting, fades and levels, markers and regions, undo, and saving the result. For editing by frequency in the spectrogram, see [Spectral editing and repair](03-spectral-editing-and-repair.md).

## Look at the file

The **View** menu chooses what the editor shows:

| View | Keys | What it shows |
|---|---|---|
| Waveform | Cmd-1 | Each channel's level over time |
| Spectrogram | Cmd-2 | Each channel's frequencies over time |
| Waveform and Spectrogram | Cmd-3 | Both, one above the other; drag the line between them to size them |

To zoom:

- **Zoom in or out:** Cmd-= and Cmd--.
- **Show the whole file:** Cmd-0.
- **Fill the view with the selection:** Cmd-E.
- **Use the buttons:** the toolbar and the strip under the editor have them too.
- **See quiet audio:** zoom in vertically with Option-Cmd-=, or Option-scroll over the waveform, up to 64 times. The level the lane's edges stand for is shown at its top left. Option-Cmd-0 goes back to full scale.

## Select

The **Time tool** (A) is the one in hand to start with. With it:

- **Select a stretch:** drag across the waveform.
- **Select the whole file:** double-click, or Edit > Select All (Cmd-A).
- **Type the times exactly:** Edit > Set Selection..., or click the selection's time on the strip under the editor.
- **Move one end:**
  - Shift-Left or Shift-Right moves the end by a pixel's worth of time.
  - Shift-Cmd-Left or Shift-Cmd-Right moves the start.
- **Move or resize the whole selection:**
  - Option-Shift-Left or Option-Shift-Right steps it by its own length.
  - Option-Shift-Up or Option-Shift-Down doubles or halves it.
- **Land the ends on zero crossings,** so cuts don't click:
  - **Edit > Select at Zero Crossings** makes every end you drag or nudge land on one.
  - **Edit > Selection to Zero Crossings** moves the current selection's ends there.

Torqwave can also find things for you, from the **Edit** menu:

| Command | What it selects |
|---|---|
| Find Next Clipping | The next run of three or more samples at full scale after the cursor |
| Find Next Silence... | The next stretch quieter than a level for at least a time you set |
| Select Next Sound, Select Previous Sound | The next or previous stretch of sound between silences |
| Select Audible Span | From the first sound to the last |

## Listen

- **Space** plays the selection, or from the cursor when nothing is selected, and stops.
- **Transport > Loop Playback** (Cmd-L) plays the selection over and over. With nothing selected it loops the whole file.
- **Home** and **End** go to the start and end of the file.
- **Cmd-Left** and **Cmd-Right** jump to the previous or next edge of the selection, or to the start or end.
- **Go to Time...** (Cmd-J) jumps to a time you type. Clicking the cursor's time under the editor does the same.
- **Scrub to find a spot by ear:** while stopped, drag along the ruler above the waveform. The sound under the mouse plays as you move, faster or backwards as you drag, and the cursor stays where you let go.

What you hear goes through the effects rack if it has effects in it. See [Effects and the rack](04-effects-and-the-rack.md).

## Cut, copy and paste

The **Edit** menu has the usual commands, and some that are particular to audio:

| Command | Keys | What it does |
|---|---|---|
| Cut, Copy | Cmd-X, Cmd-C | Take or copy the selection to the clipboard |
| Paste | Cmd-V | Put the clipboard in at the cursor, or in place of the selection |
| Paste Mix | Shift-Cmd-V | Mix the clipboard into the audio that is already there, from the cursor |
| Paste to Fill Selection | | Replace the selection with the clipboard, repeated to the selection's length |
| Delete | Delete | Take the selected time out of the file |
| Repeat Selection... | | Put copies of the selection straight after it |
| Crop to Selection | Cmd-T | Keep only the selection |
| Crop with Margins... | | Keep the selection and a time either side of it |

To make part of the file quiet without changing its length, use **Effects > Silence**:

- **Silence** replaces the selection with silence.
- **Silence Outside Selection** does the same to everything else.
- **Silence Left Channel** and **Silence Right Channel** do one side only.
- **Insert Silence...** adds new silence at the cursor.
- **Trim Silence...** removes quiet from the start and end of the file.
- **Shorten Pauses...** cuts every long pause down to a length you choose.

## Fades

The quickest fade is drawn straight on the waveform:

1. Drag the small handle at the waveform's **top-left corner** to the right for a fade in. The handle at the **top-right corner**, dragged left, makes a fade out.
2. Drag up or down as you go to bend the curve.
3. Drag again to change it, as long as it is still the last edit. Double-click a handle to remove its fade.

The **Effects > Fades** menu does the same to a selection:

- **Fade In** and **Fade Out**;
- fades at both edges;
- a fade for each region;
- **Gain Ramp...** for a slope from one level to another.

**Fade Shape** in the same menu chooses the curve.

## Levels

**Effects > Level and Loudness** changes how loud the file is:

- **Gain...** raises or lowers the level by a number of decibels.
- **Normalize...** brings the peak up or down to a level. It can treat each channel on its own.
- **Normalize Each Region...** does that region by region.
- **Match Loudness...** and **Meet Delivery Spec...** set the loudness in LUFS, for broadcast and streaming. They have their own page: [Loudness and mastering](05-loudness-and-mastering.md).

## Using an effect dialog

Most effects open a dialog with sliders. They all work the same way:

| What | How |
|---|---|
| Hear the result before applying it | **Space** starts and stops the preview |
| Compare with the original | **Bypass** |
| Apply | **Return** |
| Cancel | **Escape** |
| Step a value | Mouse wheel over a slider, or Up and Down; hold Shift for tenths |
| Reset a value | Double-click its slider |
| Start from a recipe | **Presets**: 201 factory presets for common jobs, and your own |

An effect applies to the selection, or to the whole file when nothing is selected. **Shift-R** runs the last command from the Effects menu again. It is in the Edit menu, named after that command, such as **Repeat Normalize**. If the command has a dialog, the dialog opens again.

## Markers and regions

- **Add a marker:** press **M** at the cursor. Press it with a selection to make a region (a named stretch) instead.
- **Rename a marker:** double-click it.
- **Find your way:** Option-Left and Option-Right jump between markers, and Shift-Cmd-[ and Shift-Cmd-] between regions.
- **Make markers in one go:** the **Markers** menu puts them at intervals or at silences, puts regions over each sound, marks clipping, and turns markers into regions.
- **Share them:** File > Export Markers... and Import Markers... read and write them as CSV.

WAV files keep markers and regions inside the file; AIFF keeps markers.

## Undo

- **Cmd-Z** undoes and **Shift-Cmd-Z** redoes. Each file has its own history.
- **Edit > Undo History** shows every step. Click one to go back or forward to it.
- **Keep the history:** a project keeps each file's history, so undo carries on after you reopen it (see [Getting started](01-getting-started.md#save)).

## Saving what you made

| Command | What it writes |
|---|---|
| Save (Cmd-S) | The file, in its own format |
| Save As... (Shift-Cmd-S) | A new WAV, AIFF or FLAC file, at the bit depth you choose |
| Save Selection As... | Just the selection, as a new file |
| Save Each Region As... | One file for each region |
| Save Pieces Between Markers As... | One file for each piece between markers |
| Save Channels as Mono Files... | One mono file for each channel |
| Export as MP3 or AAC... | A compressed copy |
| Export with ffmpeg... | Opus, WavPack, AC-3 and others (needs ffmpeg) |

**Writing through the effects rack:** if the rack has effects in it, the save and export dialogs have a switch, on to start with, that writes the new file through them. Your open audio and the rack stay as they are, so a later plain Save still writes the audio without the effects. Save Each Region and Save Pieces Between Markers always write through the rack when it has an effect.

**Batch Process...** in the File menu puts many files through the same steps at once:

- the effects rack;
- a recorded sequence;
- a sample rate;
- mono or stereo;
- a level;
- a file type.

Your original files are left as they are.

Next: [Spectral editing and repair](03-spectral-editing-and-repair.md).
