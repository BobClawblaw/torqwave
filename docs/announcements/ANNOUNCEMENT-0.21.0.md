# Announcing Torqwave 0.21.0 on X

A draft, not posted. 0.21.0 is the first announced release since 0.19.2 (neither 0.20.0 nor 0.20.1 was announced), so this announces everything since 0.19.2. It is one long post of text, then the pictures in posts of four under it, for people to click through. An earlier draft was a thread of nine short posts; this takes its place.

The post is about 4,200 characters, so it needs an account that can post long posts. The pictures are in [compare/graphics/](../compare/graphics/) (made from the [comparison data](../compare/README.md)) and [screenshots/](../screenshots/).

## The post

```text
Torqwave 0.21.0 is out.

Torqwave is a free, open-source audio editor and DAW in one program, for macOS, Windows and Linux. You edit a file down to its spectrum, repair and master it, then build a session around it with instruments, a piano roll, automation and video, without leaving the program.

This is the first release I'm announcing since 0.19.2, so here is everything that has landed since.

IT HAS ITS OWN INSTRUMENTS NOW

• Torqwave Synth. A wavetable synth: three oscillators with warps and unison, a sub and noise, two filters, four envelopes, two LFOs, sixteen modulation slots and four macros. 3,743 patches in 29 categories. Drop any sound on an oscillator and it becomes that oscillator's wavetable.

• Decrypt. Give the synth a recording of one note, or a held chord, and it builds a patch that plays like it. It shows up to eight candidates that sound different from each other, and the path it took to get there. Close for synth sounds, looser for voices and drums.

• Torqwave Sampler. Your own sounds across the keys in zones with loops and crossfades, or cut into slices with each piece on its own key. It takes its sound straight from the editor's selection, a clip in a session, or the Sound Library.

• StringSwarm. A guitar with every string tuned to the note you play, through a fuzz, or a swarm of oscillators in place of the strings.

• Basic Drums. 30 drum sounds on pads, each with its own tune, decay, tone, drive and effects. 21 of them now come in two types, Classic and Modern, and there are twelve kits.

All of them play from the computer's keyboard, and Audition plays each sound as you choose it.

AND SOMETHING TO PLAY ON THEM

1,032 sequence presets and 116 starter kits in 37 styles, techno and industrial first, sorted by style, energy and character. Preview any of them on your own instrument, plug-ins included.

IT STARTS WHERE YOU STOPPED

Continue where you left off: quit with files open in the editor and a session in the multitrack view, and the next start brings all of it back. Every file, your place in each, the effects rack, the session, the view you were in. It keeps a note of what was open, not a copy of your audio.

ASK TORQ

Describe an edit in a sentence and a language model you run yourself plans it; nothing changes until you read the plan and press Apply. It works with any server that speaks the OpenAI API, on your own machine. New in this release: it remembers the conversation, so "now a little less" works, and it shows how much of the model's context you've used.

IN THE EDITOR

A time-view strip over the ruler that shows the whole file, a Hand tool, and Paste Clipboard as New File.

WE MEASURED OUR OWN EFFECTS, AND FIXED WHAT FELL SHORT

For the new website I rendered before-and-after clips of Torqwave's repairs and published the numbers, including the ones that weren't good enough. Three got fixed in this release:

• The Maximizer let true peaks through 0.2 dB over its ceiling. It holds the ceiling now, on the same meter you check it with.
• Click and Pop Repair missed 16 of 60 test clicks, and was rewriting clean drum hits it mistook for clicks. It finds all 60 now and leaves drums alone.
• Repair Clipping drew a smooth curve over each clipped peak. It now rebuilds from the sound around the clip. It's better; on drums it is still mostly a guess, and the page says so.

HEAR IT BEFORE YOU DOWNLOAD IT

The website has over a thousand clips rendered by Torqwave's own code with nothing added: 750 synth patches, every drum kit, factory kits and sequences, and the repairs with their measurements.
https://bobclawblaw.github.io/torqwave/

WHAT IT DOESN'T DO

No audio to MIDI, no CLAP or ARA, no Dolby Atmos. The installers aren't notarized yet, so macOS asks before it opens the first time. The Windows and Linux builds are made by automation and haven't had the hands-on time the Mac build has. I keep a feature comparison with 15 other editors and DAWs, read from their own documentation, and it lists what Torqwave lacks as plainly as what it has.

Free. Open source, AGPL-3.0. No account, no subscription.

Download: https://github.com/BobClawblaw/torqwave/releases/tag/v0.21.0
```

## The pictures, in order

Posted under the text, four to a post. Each has its alt text below.

**First four: the release, and the synth**
1. `docs/compare/graphics/1-release-by-the-numbers.png`
2. `docs/screenshots/guide-synth.png`
3. `docs/screenshots/guide-decrypt.png`
4. `docs/screenshots/guide-decrypted.png`

**Second four: the other instruments**
5. `docs/screenshots/guide-sampler.png`
6. `docs/screenshots/guide-sampler-slices.png`
7. `docs/screenshots/guide-swarm.png`
8. `docs/screenshots/guide-drums.png`

**Third four: playing, starting, editing, measuring**
9. `docs/screenshots/guide-sequencer.png`
10. `docs/screenshots/guide-continue.png`
11. `docs/screenshots/editor.png`
12. `docs/screenshots/site-quality.png`

**Last four: where it stands**
13. `docs/compare/graphics/5-where-torqwave-stands.png`
14. `docs/compare/graphics/2-against-the-editors.png`
15. `docs/compare/graphics/3-against-the-daws-1.png`
16. `docs/compare/graphics/4-against-the-daws-2.png`

A reply under them can carry the comparison's address: `https://github.com/BobClawblaw/torqwave/blob/main/docs/compare/MATRIX.md`.

## Alt text

For graphic 1, `1-release-by-the-numbers.png`:

```alt
A dark chart titled "Torqwave 0.21.0: what it adds over 0.19.2". Six tiles, each with a number and the figure for 0.19.2 under it: 3,743 synth patches (0 before), 4 built-in instruments (1 before), 1,032 sequence presets (0 before), 296 StringSwarm presets (0 before), 12 drum kits (0 before), 93 audition patterns (0 before). A list beside them, "Also new since 0.19.2": Continue where you left off, every open file and the session back at the next start; Decrypt, a synth patch from a recording of one note; Torqwave Sampler, your own sounds across the keys or cut into slices; a wavetable from any sound dropped on an oscillator; instruments played from the computer's keys; the Maximizer, Click Repair and Repair Clipping corrected.
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
The window of Basic Drums in its Warehouse kit: the kit selector and presets row along the top, thirty pads coloured by family (kicks, snares and claps, hats, toms, cymbals, hand percussion) on the left, and the selected kick's settings on the right, with its Type selector reading Modern.
```

For `docs/screenshots/site-quality.png`:

```alt
The Quality page of Torqwave's website, "Quality: shown, and measured". Under "Before and after", a panel on clicks: a pad and bell clip with 60 clicks put in, two audio players with a spectrogram each, with the clicks and repaired, and a table: 60 put in, 60 repaired, none left more than a tenth of full scale from the clean clip, and the clip 46.9 dB from the clean one against 15.0 dB with the clicks in.
```

For `docs/screenshots/guide-continue.png`:

```alt
Torqwave's window just after it starts, with nothing open: the title Torqwave over the line "Open something you were working on, or start something new", and three buttons, Continue Where You Left Off, New File and Open..., in the middle of an empty editor, with the Files panel on the left saying no files are open.
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
A dark chart titled "Where Torqwave stands, area by area". A grid with a row for each of seventeen products (Torqwave 0.21.0, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers, Steinberg WaveLab, FL Studio, Ableton Live, Bitwig Studio, Reason, REAPER, Logic Pro, Cubase, Pro Tools, Fender Studio Pro, Ardour) and a column for each of eleven areas (editing, repair, loudness, multitrack, MIDI, instruments, plug-ins, picture, library, AI help, platforms). Each cell gives how many of the area's rows the product covers. A panel at the right shows Torqwave's rows covered by release: 39 of 53 for 0.19.2, and 45 for each of 0.20.0, 0.20.1 and 0.21.0. A line under the title says it is a count of features, not of depth or maturity.
```

For graphics 2, 3 and 4, `2-against-the-editors.png`, `3-against-the-daws-1.png` and `4-against-the-daws-2.png`:

```alt
A dark table titled "Torqwave 0.21.0 and the audio editors". Rows are features such as spectral selection, noise reduction, stem separation, loudness and delivery checks; columns are Torqwave 0.21.0, Torqwave 0.19.2, Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers and Steinberg WaveLab. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked partial for stem separation and for turning a typed request into edits.
```

```alt
A dark table titled "Torqwave 0.21.0 and the DAWs, 1 of 2". Rows are features such as comping, clip launching, a piano roll, built-in instruments, a sampler, audio to MIDI, CLAP and ARA 2; columns are Torqwave 0.21.0, Torqwave 0.19.2, FL Studio, Ableton Live, Bitwig Studio, Reason and REAPER. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked no for audio to MIDI, Dolby Atmos, ARA 2 and CLAP.
```

```alt
A dark table titled "Torqwave 0.21.0 and the DAWs, 2 of 2", with the same rows; columns are Torqwave 0.21.0, Torqwave 0.19.2, Logic Pro, Cubase, Pro Tools, Fender Studio Pro and Ardour. Each cell is marked yes, partial, no or not confirmed. Torqwave is marked no for audio to MIDI, Dolby Atmos, ARA 2 and CLAP, and partial for stem separation, where four of these five are marked yes.
```

## Notes for whoever posts it

- The release is published and its Mac, Windows and Linux packages are on the release page, so "is out" and "for macOS, Windows and Linux" are true. The Windows and Linux packages are built by the workflows and not tried by hand, which the post says.
- The figures: 3,743 patches, 30 sounds with 21 in two types, twelve kits, 1,032 sequence presets, 116 starter kits and 37 styles are the release's own counts. "16 of 60" and "all 60" are Click and Pop Repair on a pad and bell clip with 60 clicks put in; in a drum loop it repairs 58 of 60, which the post does not claim. "0.2 dB over" is 0.21 dB on Torqwave's loudness meter. "Over a thousand clips" is 1,065 on the site, 750 of them synth patches.
- The post says "I": it is written to be posted by the project's author.
- It names no other product, and says what Torqwave lacks.
