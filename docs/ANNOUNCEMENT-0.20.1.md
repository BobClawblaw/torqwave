# Announcing Torqwave 0.20.1 on X

A draft, not posted. One main post and a thread of three, for the day 0.20.1 is released, with the comparison's graphics and their alt text. Each post is the plain text in its box: copy it from there. `<release link>` stands for the release's address (`https://github.com/BobClawblaw/torqwave/releases/tag/v0.20.1` once it exists); X counts any link as 23 characters.

Check the lengths after any change:

```bash
python3 docs/compare/generate.py --posts docs/ANNOUNCEMENT-0.20.1.md
```

## The main post

One post, with graphic 1 (`compare/graphics/1-release-by-the-numbers.png`): what 0.20.1 adds over 0.19.2, with Continue where you left off first in its list. A screenshot of File > Settings... > Start with the new choice selected would suit it too; none is made yet.

```text
Torqwave 0.20.1 is out.

New: Continue where you left off. Quit with five files open in the editor and a session in the multitrack view, and the next start brings all of it back: every file, your place in each, the session, the view you were in.

<release link>
```

## The thread

**1 of 3**, with graphic 1 (`compare/graphics/1-release-by-the-numbers.png`).

```text
Torqwave 0.20.1 is out. It is a free, open-source audio editor and DAW in one, and this release is about starting where you stopped.

Settings > Start has a new choice: Continue where you left off.

<release link>
```

**2 of 3**, with no graphic: a screenshot of the editor and the session as they come back would suit it; none is made yet.

```text
What comes back: every file that was open in the editor, in order, each with its selection, cursor, zoom and scroll. The effects rack. The session, with its cursor, time range and zoom. The editor or the multitrack view in front, as it was.
```

**3 of 3**, with graphic 5 (`compare/graphics/5-where-torqwave-stands.png`) and graphics 2, 3 and 4 (the editors, and the DAWs in two parts) to fill the post's four images, if the comparison is to go out with this release. The post does not speak of the comparison, so either leave the four off or add the optional post below.

```text
It keeps a note of what was open, not a copy of your audio. A file that has moved or a drive that isn't connected is listed, the rest opens, and it is tried again at the next start.

Also a command: File > Continue Where You Left Off.

<release link>
```

**Optional, 4 of 4**, with graphics 5, 2, 3 and 4 in place of putting them on post 3. It is the 0.20.0 thread's last post, which was not posted with these pictures' 0.20.1 versions.

```text
Where does it stand? We keep a feature comparison with 15 other editors and DAWs, each cell read from that product's own documentation, and update it every release.

It shows what Torqwave lacks too: no audio to MIDI, no CLAP, ARA or Dolby Atmos, and installers not yet notarized.
```

## Alt text

For graphic 1, `1-release-by-the-numbers.png`:

```alt
A dark chart titled "Torqwave 0.20.1: what it adds over 0.19.2". Six tiles, each with a number and the figure for 0.19.2 under it: 3,743 synth patches (0 before), 4 built-in instruments (1 before), 1,018 sequence presets (0 before), 296 StringSwarm presets (0 before), 30 drum sounds (0 before), 85 audition patterns (0 before). A list beside them, "Also new since 0.19.2": Continue where you left off, every open file and the session back at the next start; Decrypt, a synth patch from a recording of one note; Torqwave Sampler, your own sounds across the keys or cut into slices; a wavetable from any sound dropped on an oscillator; instruments played from the computer's keys; a time-view strip, a Hand tool and Paste Clipboard as New File.
```

For graphic 5, `5-where-torqwave-stands.png`:

```alt
A dark chart titled "Where Torqwave stands, area by area". A grid with a row for each of seventeen products (Torqwave 0.20.1, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers, Steinberg WaveLab, FL Studio, Ableton Live, Bitwig Studio, Reason, REAPER, Logic Pro, Cubase, Pro Tools, Fender Studio Pro, Ardour) and a column for each of eleven areas (editing, repair, loudness, multitrack, MIDI, instruments, plug-ins, picture, library, AI help, platforms). Each cell gives how many of the area's rows the product covers. A panel at the right shows Torqwave's rows covered by release: 39 of 53 for 0.19.2, 45 for 0.20.0 and 45 for 0.20.1. A line under the title says it is a count of features, not of depth or maturity.
```

For graphics 2, 3 and 4, `2-against-the-editors.png`, `3-against-the-daws-1.png` and `4-against-the-daws-2.png`:

```alt
A dark table titled "Torqwave 0.20.1 and the audio editors". Rows are features such as spectral selection, noise reduction, stem separation, loudness and delivery checks; columns are Torqwave 0.20.1, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers and Steinberg WaveLab. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked partial for stem separation and for turning a typed request into edits.
```

```alt
A dark table titled "Torqwave 0.20.1 and the DAWs, 1 of 2". Rows are features such as comping, clip launching, a piano roll, built-in instruments, a sampler, audio to MIDI, CLAP and ARA 2; columns are Torqwave 0.20.1, Torqwave 0.19.2, FL Studio, Ableton Live, Bitwig Studio, Reason and REAPER. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked no for audio to MIDI, Dolby Atmos, ARA 2 and CLAP.
```

```alt
A dark table titled "Torqwave 0.20.1 and the DAWs, 2 of 2", with the same rows; columns are Torqwave 0.20.1, Torqwave 0.19.2, Logic Pro, Cubase, Pro Tools, Fender Studio Pro and Ardour. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked no for audio to MIDI, Dolby Atmos, ARA 2 and CLAP, and partial for stem separation, where four of these five are marked yes.
```

## Notes for whoever posts it

- The graphics say 0.20.1 "is about to be released" until the snapshot's `status` is set to `released` and the generator run again (`python3 docs/compare/generate.py`): do that on the day, with the day's date in `as_of`.
- Graphic 1 sets 0.20.1 against 0.19.2, not 0.20.0: its numbers are what the 0.20 line added, and only the first line of its list is new in 0.20.1 itself. The posts above speak of 0.20.1's own change alone.

- Nothing here is posted until the release is published: "is out" is true only then.
- 0.20.0's Linux package failed to build (a date format the Linux build tool could not read), and 0.20.1 fixes that. Say "for macOS, Windows and Linux" only once the 0.20.1 Windows and Linux packages are on the release page.
- What it does not bring back, if someone asks: changes you chose not to save, files never saved, undo histories, and open instrument, plug-in and tool windows. A project keeps more.
- The feature was tested by its own suite and tried once in the built app on made-up files. Say so if asked how it was checked.
- No other product is named.
