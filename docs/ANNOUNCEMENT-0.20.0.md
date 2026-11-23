# Announcing Torqwave 0.20.0 on X

A draft, not posted. It holds one main post, a thread of six, and alt text for every image, for the day 0.20.0 is released. Each post is the plain text in its box: copy it from there. `<release link>` stands for the release's address (`https://github.com/BobClawblaw/torqwave/releases/tag/v0.20.0` once the tag exists); X counts any link as 23 characters, and the counts below do too.

Check the lengths after any change:

```bash
python3 docs/compare/generate.py --posts docs/ANNOUNCEMENT-0.20.0.md
```

The graphics are in [compare/graphics/](compare/graphics/), made from the [comparison data](compare/README.md); the screenshots are in [screenshots/](screenshots/). Before posting, run the generator again so the graphics carry the day's date and say the release is out, and check that every number below still matches the release notes.

## The main post

One post, with graphic 1 (`compare/graphics/1-release-by-the-numbers.png`).

```text
Torqwave 0.20.0 is out. The free, open-source audio editor and DAW gets its own instruments:

• Torqwave Synth: a wavetable synth, 3,743 patches
• Decrypt: a patch from a recording of one note
• StringSwarm and Basic Drums
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

**3 of 6**, with the synth's window on its Decrypt page (take this screenshot from the release build; the page's display is still changing).

```text
Decrypt builds a synth patch from a recording of one note. It finds the pitch, plays every factory patch against the recording, then changes the nearest ones round after round.

Close for sounds a synth made. Loose for voices, drums and samples: a patch in their spirit.
```

**4 of 6**, with StringSwarm's and Basic Drums' windows (`screenshots/guide-swarm.png`, `screenshots/guide-drums.png`).

```text
Two more instruments. StringSwarm: a guitar with every string tuned to the note you play, through a fuzz, or a swarm of oscillators. Basic Drums: 30 sounds on pads, for every General MIDI drum.

Both play from the computer's keys. Audition plays each sound as you choose it.
```

**5 of 6**, with the Sequencer workspace (`screenshots/guide-sequencer.png`) and the editor (`screenshots/editor.png`).

```text
Something to play on them: 1,018 sequence presets and 116 starter kits in 37 styles, sorted by style, energy and character.

In the editor: a time-view strip over the ruler, a Hand tool, and Paste Clipboard as New File.
```

**6 of 6**, with graphic 4 (`compare/graphics/4-where-torqwave-stands.png`), and graphics 2 and 3 if there is room for three images.

```text
Where does it stand? We keep a feature comparison with ten other editors and DAWs, each cell read from that product's own documentation, and update it every release.

It shows what Torqwave lacks too: no sampler, no audio to MIDI, no CLAP or ARA, and installers not yet notarized.
```

A reply under the thread can carry the comparison's address once the branch is on GitHub: `https://github.com/BobClawblaw/torqwave/blob/main/docs/compare/MATRIX.md`.

## Alt text

For graphic 1, `1-release-by-the-numbers.png`:

```alt
A dark chart titled "Torqwave 0.20.0: what it adds over 0.19.2". Six tiles, each with a number and the figure for 0.19.2 under it: 3,743 synth patches (0 before), 3 built-in instruments (1 before), 1,018 sequence presets (0 before), 296 StringSwarm presets (0 before), 30 drum sounds (0 before), 85 audition patterns (0 before). A list beside them, "Also new in 0.20.0": Decrypt, a synth patch from a recording of one note; a wavetable from any sound dropped on an oscillator; instruments played from the computer's keys; instruments and plug-ins follow the session's tempo; a time-view strip, a Hand tool and Paste Clipboard as New File.
```

For `screenshots/guide-synth.png`:

```alt
The window of Torqwave Synth with the bass patch Turbine Throat: the presets row with the Knobs and Decrypt switch, three wavetable oscillators each showing its wave, the sub and noise, two filters with their curves, the amp envelope, four macro knobs, the modulation slots, and the playing keys along the bottom.
```

For the Decrypt screenshot (write this when the picture is taken, saying what it shows: the recording dropped in, the chain of steps and how near the patch measures).

For `screenshots/guide-swarm.png` and `screenshots/guide-drums.png`:

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

For graphic 4, `4-where-torqwave-stands.png`:

```alt
A dark chart titled "Where Torqwave stands, area by area". A grid with a row for each of twelve products (Torqwave 0.20.0, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers, Steinberg WaveLab, FL Studio, REAPER, Ableton Live, Bitwig Studio, Ardour) and a column for each of eleven areas (editing, repair, loudness, multitrack, MIDI, instruments, plug-ins, picture, library, AI help, platforms). Each cell gives how many of the area's rows the product covers. A panel at the right shows Torqwave's rows covered by release, with empty slots for releases to come. A line under the title says it is a count of features, not of depth or maturity.
```

For graphics 2 and 3, `2-against-the-editors.png` and `3-against-the-daws.png`:

```alt
A dark table titled "Torqwave 0.20.0 and the audio editors". Rows are features such as spectral selection, noise reduction, stem separation, loudness and delivery checks; columns are Torqwave 0.20.0, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers and Steinberg WaveLab. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked partial for stem separation and for turning a typed request into edits.
```

```alt
A dark table titled "Torqwave 0.20.0 and the DAWs". Rows are features such as comping, clip launching, a piano roll, built-in instruments, a sampler, audio to MIDI, CLAP and ARA 2; columns are Torqwave 0.20.0, Torqwave 0.19.2, FL Studio, REAPER, Ableton Live, Bitwig Studio and Ardour. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked no for a built-in sampler, audio to MIDI, Dolby Atmos, ARA 2 and CLAP.
```

## Notes for whoever posts it

- It is written for the day of the release. Until then nothing here is posted, and "is out" is not yet true.
- No other product is named in the posts, and none is called worse. The comparison is offered as a table to read, with what Torqwave lacks in the same post.
- Two hashtags at most if any are added; the draft has none.
- The Windows and Linux packages of 0.20.0 are built by the workflows and, by the release notes, were not tried by hand. If someone asks, say so.
