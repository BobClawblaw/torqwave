# Announcing Torqwave 0.20.0 on X

A draft, not posted. It holds one main post, a thread of six, and alt text for every image, for the day 0.20.0 is released. Each post is the plain text in its box: copy it from there. `<release link>` stands for the release's address (`https://github.com/BobClawblaw/torqwave/releases/tag/v0.20.0` once the tag exists); X counts any link as 23 characters, and the counts below do too.

Check the lengths after any change:

```bash
python3 docs/compare/generate.py --posts docs/ANNOUNCEMENT-0.20.0.md
```

The graphics are in [compare/graphics/](compare/graphics/), made from the [comparison data](compare/README.md); the screenshots are in [screenshots/](screenshots/). The graphics were regenerated for release day: they carry the date 2026-11-24 (the other products' cells were read on 2026-11-23, as the matrix says for each) and say 0.20.0 is released. If the release goes out on another day, set `as_of` and Torqwave's `version_date` in `compare/data/snapshots/0.20.0.json` and run the generator again. Check that every number below still matches the release notes.

## The main post

One post, with graphic 1 (`compare/graphics/1-release-by-the-numbers.png`).

```text
Torqwave 0.20.0 is out. The free, open-source audio editor and DAW gets its own instruments:

• Torqwave Synth: wavetable, 3,743 patches
• Decrypt: a patch from a recording of one note
• Sampler, StringSwarm and Basic Drums
• 1,018 sequence presets

<release link>
```

## The thread

**1 of 6**, with graphic 1 (`compare/graphics/1-release-by-the-numbers.png`).

```text
Torqwave 0.20.0 is out. It is a free, open-source audio editor and DAW in one, for macOS, Windows and Linux, and this release gives it its own instruments and something to play on them.

What is new, in six posts.

<release link>
```

**2 of 6**, with the synth's window (`screenshots/guide-synth.png`).

```text
Torqwave Synth: three wavetable oscillators with warps and unison, a sub and noise, two filters, four envelopes, two LFOs, sixteen modulation slots and four macros.

3,743 patches in 29 categories. Drop any sound on an oscillator and it becomes that oscillator's wavetable.
```

**3 of 6**, with the synth's window on its Decrypt page (`screenshots/guide-decrypt.png`, drawn by the tests from a factory bell note).

```text
Decrypt builds a synth patch from a recording of one note. It finds the pitch, plays every factory patch against the recording, then changes the nearest ones round after round.

Close for sounds a synth made. Loose for voices, drums and samples: a patch in their spirit.
```

**4 of 6**, with Torqwave Sampler's, StringSwarm's and Basic Drums' windows (`screenshots/guide-sampler.png`, `screenshots/guide-swarm.png`, `screenshots/guide-drums.png`). The sampler's picture is drawn by the tests without the playing keys; retake it from the release build if there is time.

```text
Three more instruments. Torqwave Sampler: your own sounds across the keys, or cut into slices. StringSwarm: a guitar with every string tuned to the note you play, through a fuzz, or a swarm of oscillators. Basic Drums: 30 sounds on pads.

All play from the computer's keys.
```

**5 of 6**, with the Sequencer workspace (`screenshots/guide-sequencer.png`) and the editor (`screenshots/editor.png`).

```text
Something to play on them: 1,018 sequence presets and 116 starter kits in 37 styles, sorted by style, energy and character.

In the editor: a time-view strip over the ruler, a Hand tool, and Paste Clipboard as New File.
```

**6 of 6**, with graphic 5 (`compare/graphics/5-where-torqwave-stands.png`), and graphics 2, 3 and 4 (the editors, and the DAWs in two parts) to fill the post's four images.

```text
Where does it stand? We keep a feature comparison with 15 other editors and DAWs, each cell read from that product's own documentation, and update it every release.

It shows what Torqwave lacks too: no audio to MIDI, no CLAP, ARA or Dolby Atmos, and installers not yet notarized.
```

A reply under the thread can carry the comparison's address once the branch is on GitHub: `https://github.com/BobClawblaw/torqwave/blob/main/docs/compare/MATRIX.md`.

## Alt text

For graphic 1, `1-release-by-the-numbers.png`:

```alt
A dark chart titled "Torqwave 0.20.0: what it adds over 0.19.2". Six tiles, each with a number and the figure for 0.19.2 under it: 3,743 synth patches (0 before), 4 built-in instruments (1 before), 1,018 sequence presets (0 before), 296 StringSwarm presets (0 before), 30 drum sounds (0 before), 85 audition patterns (0 before). A list beside them, "Also new in 0.20.0": Decrypt, a synth patch from a recording of one note; Torqwave Sampler, your own sounds across the keys or cut into slices; a wavetable from any sound dropped on an oscillator; instruments played from the computer's keys; instruments and plug-ins follow the session's tempo; a time-view strip, a Hand tool and Paste Clipboard as New File.
```

For `screenshots/guide-synth.png`:

```alt
The window of Torqwave Synth with the bass patch Turbine Throat: the presets row with the Knobs and Decrypt switch, three wavetable oscillators each showing its wave, the sub and noise, two filters with their curves, the amp envelope, four macro knobs, the modulation slots, and the playing keys along the bottom.
```

For `screenshots/guide-decrypt.png`:

```alt
Torqwave Synth's Decrypt page after a search on a bell note. A round scope with the recording's signature in the middle and eight candidate patches round it, each with how near it measures; the nearest is ringed at 99 %. A numbered path of three steps crosses the scope. A panel on the right shows the lock, a bar each for oscillators, filter, envelopes and modulation, and the buttons Stop, Take and Again. Along the bottom, a chain: Play, the signal, and three links at 23, 37 and 99 %.
```

For `screenshots/guide-sampler.png`, `screenshots/guide-swarm.png` and `screenshots/guide-drums.png`:

```alt
The window of Torqwave Sampler playing a held note: the Keys and Slices switch and the presets row along the top, the sample's waveform with its start, end and a shaded loop, the zone drawn across the keys either side of its root, and the zone's, amp's, filter's, LFO's and voice's settings below.
```

```alt
The window of StringSwarm with the preset Tremolo Swarm Wall: twelve strings all tuned to G across three octaves, each a few cents apart, the fuzz amp with its Drive, Tone, Level and Velocity knobs, the strum and layers controls, and the playing keys along the bottom.
```

```alt
The window of Basic Drums in its Industrial kit: the kit selector and presets row along the top, thirty pads coloured by family (kicks, snares and claps, hats, toms, cymbals, hand percussion) on the left, and the selected kick's settings on the right.
```

For `screenshots/guide-sequencer.png` and `screenshots/editor.png`:

```alt
Torqwave's Sequencer workspace: the Sequence Library on the left, the Sequencer with a three-bar hook's notes and its Torqwave Synth voice, and the Idea Board below with the Chorus scene playing.
```

```alt
Torqwave's editor: a stereo file as waveform and spectrogram, the time-view strip over the ruler showing the whole file with a frame round the part in view, the tool strip under the waveform with the Hand tool, and the Files, Effects Rack, Selection and Levels panels around it.
```

For graphic 5, `5-where-torqwave-stands.png`:

```alt
A dark chart titled "Where Torqwave stands, area by area". A grid with a row for each of seventeen products (Torqwave 0.20.0, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers, Steinberg WaveLab, FL Studio, Ableton Live, Bitwig Studio, Reason, REAPER, Logic Pro, Cubase, Pro Tools, Fender Studio Pro, Ardour) and a column for each of eleven areas (editing, repair, loudness, multitrack, MIDI, instruments, plug-ins, picture, library, AI help, platforms). Each cell gives how many of the area's rows the product covers. A panel at the right shows Torqwave's rows covered by release, with empty slots for releases to come. A line under the title says it is a count of features, not of depth or maturity.
```

For graphics 2, 3 and 4, `2-against-the-editors.png`, `3-against-the-daws-1.png` and `4-against-the-daws-2.png`:

```alt
A dark table titled "Torqwave 0.20.0 and the audio editors". Rows are features such as spectral selection, noise reduction, stem separation, loudness and delivery checks; columns are Torqwave 0.20.0, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers and Steinberg WaveLab. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked partial for stem separation and for turning a typed request into edits.
```

```alt
A dark table titled "Torqwave 0.20.0 and the DAWs, 1 of 2". Rows are features such as comping, clip launching, a piano roll, built-in instruments, a sampler, audio to MIDI, CLAP and ARA 2; columns are Torqwave 0.20.0, Torqwave 0.19.2, FL Studio, Ableton Live, Bitwig Studio, Reason and REAPER. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked no for audio to MIDI, Dolby Atmos, ARA 2 and CLAP.
```

```alt
A dark table titled "Torqwave 0.20.0 and the DAWs, 2 of 2", with the same rows; columns are Torqwave 0.20.0, Torqwave 0.19.2, Logic Pro, Cubase, Pro Tools, Fender Studio Pro and Ardour. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked no for audio to MIDI, Dolby Atmos, ARA 2 and CLAP, and partial for stem separation, where four of these five are marked yes.
```

## Notes for whoever posts it

- It is written for the day of the release. Nothing here is posted until the release is published: "is out" is true only then.
- No other product is named in the posts, and none is called worse. The comparison is offered as a table to read, with what Torqwave lacks in the same post.
- Torqwave Sampler ships in 0.20.0. It was tried by hand in the app, and the two problems found that way (fix log 98 and 99) were fixed before the release.
- Two hashtags at most if any are added; the draft has none.
- The Windows and Linux packages of 0.20.0 are built by the workflows and, by the release notes, were not tried by hand. If someone asks, say so.
