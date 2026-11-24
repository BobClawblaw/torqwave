# Announcing Torqwave 0.20.1 on X

**Superseded by [ANNOUNCEMENT-0.21.0.md](ANNOUNCEMENT-0.21.0.md): 0.20.1 was not announced.** A draft, not posted. 0.20.1 is the major release: this announces everything since 0.19.2 (0.20.0 was a baseline, published with the Mac disk image alone and not announced). It holds one main post, a thread of eight, and alt text for every image. Each post is the plain text in its box: copy it from there. `<release link>` stands for the release's address (`https://github.com/BobClawblaw/torqwave/releases/tag/v0.20.1` once it exists); X counts any link as 23 characters, and the counts below do too.

Check the lengths after any change:

```bash
python3 docs/compare/generate.py --posts docs/announcements/ANNOUNCEMENT-0.20.1.md
```

The graphics are in [compare/graphics/](../compare/graphics/), made from the [comparison data](../compare/README.md); the screenshots are in [screenshots/](../screenshots/).

## The main post

One post, with graphic 1 (`docs/compare/graphics/1-release-by-the-numbers.png`).

```text
Torqwave 0.20.1 is out. New in the free, open-source audio editor and DAW:

• Torqwave Synth: wavetable, 3,743 patches
• Decrypt: a patch from a recording
• Sampler, StringSwarm, Basic Drums
• 1,018 sequence presets
• A start where you left off

<release link>
```

## The thread

**1 of 8**, with graphic 1 (`docs/compare/graphics/1-release-by-the-numbers.png`).

```text
Torqwave 0.20.1 is out. It is a free, open-source audio editor and DAW in one, and this release gives it its own instruments, something to play on them, and a start that brings back what you had open.

What is new since 0.19.2, in eight posts.

<release link>
```

**2 of 8**, with the synth's window (`docs/screenshots/guide-synth.png`).

```text
Torqwave Synth: three wavetable oscillators with warps and unison, a sub and noise, two filters, four envelopes, two LFOs, sixteen modulation slots and four macros.

3,743 patches in 29 categories. Drop any sound on an oscillator and it becomes that oscillator's wavetable.
```

**3 of 8**, with the Decrypt page and the Decrypted page (`docs/screenshots/guide-decrypt.png`, `docs/screenshots/guide-decrypted.png`).

```text
Decrypt builds a synth patch from a recording of one note, or of a chord, matched as a chord. It shows up to eight candidates that sound different. Take one, name it, and it is kept on the Decrypted page by category.

Close for synth sounds; looser for voices and drums.
```

**4 of 8**, with Torqwave Sampler's window in Keys and in Slices (`docs/screenshots/guide-sampler.png`, `docs/screenshots/guide-sampler-slices.png`).

```text
Torqwave Sampler plays your own sounds: across the keys in zones with loops and crossfades, or cut into slices with each piece on its own key.

It takes its sound straight from the editor's selection, a clip in a session, the Files panel or the Sound Library.
```

**5 of 8**, with StringSwarm's and Basic Drums' windows (`docs/screenshots/guide-swarm.png`, `docs/screenshots/guide-drums.png`).

```text
Two more instruments. StringSwarm: a guitar with every string tuned to the note you play, through a fuzz, or a swarm of oscillators. Basic Drums: 30 sounds on pads, for every General MIDI drum.

All four play from the computer's keys. Audition plays each sound as you choose it.
```

**6 of 8**, with the Sequencer workspace (`docs/screenshots/guide-sequencer.png`) and the editor (`docs/screenshots/editor.png`).

```text
Something to play on them: 1,018 sequence presets and 116 starter kits in 37 styles, sorted by style, energy and character.

In the editor: a time-view strip over the ruler, a Hand tool, and Paste Clipboard as New File.
```

**7 of 8**, with no picture yet: the home screen with its Continue Where You Left Off button, or File > Settings... > Start with the new choice selected, would suit it (the guide's pictures of both are from before the feature).

```text
Continue where you left off: quit with files open in the editor and a session in the multitrack view, and the next start brings all of it back. Every file, your place in each, the effects rack, the session, the view you were in.

It keeps a note of what was open, not your audio.
```

**8 of 8**, with graphic 5 (`docs/compare/graphics/5-where-torqwave-stands.png`), and graphics 2, 3 and 4 (the editors, and the DAWs in two parts) to fill the post's four images.

```text
Where does it stand? We keep a feature comparison with 15 other editors and DAWs, each cell read from that product's own documentation, and update it every release.

It shows what Torqwave lacks too: no audio to MIDI, no CLAP, ARA or Dolby Atmos, and installers not yet notarized.
```

A reply under the thread can carry the comparison's address: `https://github.com/BobClawblaw/torqwave/blob/main/docs/compare/MATRIX.md`.

## Alt text

For graphic 1, `1-release-by-the-numbers.png`:

```alt
A dark chart titled "Torqwave 0.20.1: what it adds over 0.19.2". Six tiles, each with a number and the figure for 0.19.2 under it: 3,743 synth patches (0 before), 4 built-in instruments (1 before), 1,018 sequence presets (0 before), 296 StringSwarm presets (0 before), 30 drum sounds (0 before), 85 audition patterns (0 before). A list beside them, "Also new since 0.19.2": Continue where you left off, every open file and the session back at the next start; Decrypt, a synth patch from a recording of one note; Torqwave Sampler, your own sounds across the keys or cut into slices; a wavetable from any sound dropped on an oscillator; instruments played from the computer's keys; a time-view strip, a Hand tool and Paste Clipboard as New File.
```

For `docs/screenshots/guide-synth.png`:

```alt
The window of Torqwave Synth with the bass patch Turbine Throat: the presets row with the Knobs and Decrypt switch, three wavetable oscillators each showing its wave, the sub and noise, two filters with their curves, the amp envelope, four macro knobs, the modulation slots, and the playing keys along the bottom.
```

For `docs/screenshots/guide-decrypt.png` and `docs/screenshots/guide-decrypted.png`:

```alt
Torqwave Synth's Decrypt page after a search on a bell note. A round scope with the recording's signature in the middle and eight candidate patches round it, each with how near it measures; the nearest is ringed at 99 %. A numbered path of three steps crosses the scope. A panel on the right shows the lock, a bar each for oscillators, filter, envelopes and modulation, and the buttons Stop, Take and Again. Along the bottom, a chain: Play, the signal, and three links at 23, 37 and 99 %.
```

```alt
Torqwave Synth's Decrypted page listing twelve patches taken from Decrypt: a row each with its name, category, lock, the recording it came from and the date taken, sorted by name, one row on the keys; a filter box at the top right; and Category, Rename and Delete along the bottom. The patches in the picture are made up for it.
```

For `docs/screenshots/guide-sampler.png` and `docs/screenshots/guide-sampler-slices.png`:

```alt
The window of Torqwave Sampler playing a held note: the Keys and Slices switch and the presets row along the top, the sample's waveform with its start, end and a shaded loop, the zone drawn across the keys either side of its root, and the zone's, amp's, filter's, LFO's and voice's settings below.
```

```alt
The window of Torqwave Sampler in Slices: a two-bar drum loop in the waveform with a cut at each of its 20 hits, each slice numbered, the fifth lit; the Slices panel with the source set to Hits and 20 pads; and the selected slice's settings, with Slice Again and its Sensitivity knob.
```

For `docs/screenshots/guide-swarm.png` and `docs/screenshots/guide-drums.png`:

```alt
The window of StringSwarm with the preset Tremolo Swarm Wall: twelve strings all tuned to G across three octaves, each a few cents apart, the fuzz amp with its Drive, Tone, Level and Velocity knobs, the strum and layers controls, and the playing keys along the bottom.
```

```alt
The window of Basic Drums in its Industrial kit: the kit selector and presets row along the top, thirty pads coloured by family (kicks, snares and claps, hats, toms, cymbals, hand percussion) on the left, and the selected kick's settings on the right.
```

For `docs/screenshots/guide-sequencer.png` and `docs/screenshots/editor.png`:

```alt
Torqwave's Sequencer workspace: the Sequence Library on the left, the Sequencer with a three-bar hook's notes and its Torqwave Synth voice, and the Idea Board below with the Chorus scene playing.
```

```alt
Torqwave's editor: a stereo file as waveform and spectrogram, the time-view strip over the ruler showing the whole file with a frame round the part in view, the tool strip under the waveform with the Hand tool, and the Files, Effects Rack, Selection and Levels panels around it.
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

- The release is published (2026-11-24), so "is out" is true.
- The graphics were drawn again after the release and say 0.20.1 is released.
- Graphic 1 sets 0.20.1 against 0.19.2, as the posts do. Only Continue where you left off is new in 0.20.1 itself; the rest came in 0.20.0.
- Platforms: the 0.20.1 Windows and Linux packages are on the release page beside the Mac disk image, so the posts may say "for macOS, Windows and Linux". They are built by the workflows and, by the release notes, not tried by hand. 0.20.0's Windows and Linux packages failed to build.
- How it was tried, if someone asks. Torqwave Sampler was tried by hand in the app before release (its window, sounds dragged from the Files panel, loops forward and back, its settings), and the two problems found that way were fixed (fix log 98 and 99). Decrypt was used by hand with real recordings and a local model. Continue where you left off was tested by its own suite and tried once in the built app on made-up files.
- The sampler's and Decrypt's pictures are drawn by Torqwave's tests; the sampler's lack the strip of playing keys the window has in the app.
- What Continue where you left off does not bring back, if someone asks: changes you chose not to save, files never saved, undo histories, and open instrument, plug-in and tool windows. A project keeps more.
- No other product is named in the posts, and none is called worse. Two hashtags at most if any are added; the draft has none.
