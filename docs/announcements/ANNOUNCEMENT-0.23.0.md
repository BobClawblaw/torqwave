# Announcing Torqwave 0.23.0

Ready-to-post copy for X (both a short 280-character thread checked by `python3 docs/compare/generate.py --posts` and a long-post version) plus alt text for the 0.23.0 comparison graphics in [compare/graphics/](../compare/graphics/).

## Thread (under 280 chars per post)

**Post 1 — attach `1-release-by-the-numbers.png`:**

```text
Torqwave 0.23.0 is out: free, open-source audio editor + DAW for macOS, Windows & Linux.

New: 6 Live Creative FX, 4 MIDI FX, Audio to MIDI, CLAP 1.2, Synth Arp + 4 scenes + Curve LFO, Sampler SFZ + Warp, Drums 8-bus out & Track Modulators.

https://github.com/BobClawblaw/torqwave/releases/tag/v0.23.0
```

**Post 2 — attach `5-where-torqwave-stands.png`:**

```text
In our feature comparison against 15 editors and DAWs (read from each product's own docs), 0.23.0 moves Torqwave from 45 to 47 of 53 rows with native CLAP 1.2 hosting and Audio to MIDI (melody via YIN + 3-band drum extraction).

Matrix: https://github.com/BobClawblaw/torqwave/blob/main/docs/compare/MATRIX.md
```

**Post 3 — attach `3-against-the-daws-1.png` and `4-against-the-daws-2.png`:**

```text
Built-in plug-ins grow from 22 to 32:
• Live FX: Delay, 4x Distortion, Auto-Filter, Volume Shaper, Noise Gate, Tremolo & Auto-Pan
• MIDI FX: Arpeggiator, Chord Trigger, Scale Lock, Note Echo

Plus Piano Roll ratchets, micro-timing, grooves & ghost notes.
```

**Post 4 — attach `2-against-the-editors.png`:**

```text
Torqwave Synth keeps 100% sample-for-sample identity on all 3,627 prior patches while adding LFOs 3-4, 24 mod slots, Slot Depth modulation, 16-node Curve LFOs, a built-in Arp, 4-scene Evolve morphing & Complex Decrypt.

Hear 1,235 clips: https://bobclawblaw.github.io/torqwave/
```

**Post 5 (honesty / what is still missing):**

```text
What Torqwave still doesn't have: Dolby Atmos, ARA 2, a notation editor, full neural stem export, or notarized macOS/Windows installers yet.

2,782 automated checks passed on Apple Silicon & Intel (Rosetta), plus headless CI tests on Windows & Linux. AGPL-3.0, no account.
```

## Alt text for the graphics

For `docs/compare/graphics/1-release-by-the-numbers.png`:

```alt
A dark chart titled "Torqwave 0.23.0: what it adds over 0.22.0". Six tiles comparing 0.23.0 against 0.22.0: 32 built-in plug-ins (22 before), 116 Complex patches (58 before), 24 synth mod slots (16 before), 47 matrix coverage rows of 53 (45 before, adding Audio to MIDI and CLAP 1.2), 9 drum output buses (Main plus 8 stereo Aux buses, 1 before), and 2,782 automated checks (2,741 before). A panel beside them lists what is new: 6 live Creative FX, 4 live MIDI FX, Synth 4 LFOs/24 slots/Slot Depth/Curve LFO/Arp/4 scenes, Audio to MIDI (melody and drums), Piano Roll ratchets/micro-timing/grooves/ghost notes, Sampler SFZ/round robin/Warp and Drums 8-bus multi-out, and Track Modulators, CLAP 1.2 and headless CI tests.
```

For `docs/compare/graphics/5-where-torqwave-stands.png`:

```alt
A dark comparison chart titled "Where Torqwave stands, area by area" listing Torqwave 0.23.0, Torqwave 0.22.0, five audio editors and ten DAWs across eleven feature areas. Torqwave 0.23.0 covers 47 of 53 tracked rows (up from 45 in 0.22.0), including 5/5 Editing, 4/5 Repair, 5/5 Loudness, 7/9 Multitrack, 5/6 MIDI (up from 4/6 with Audio to MIDI), 6/6 Instruments, 4/5 Plug-ins (up from 3/5 with CLAP 1.2), 4/4 Picture, 1/1 Library, 2/3 AI help and 4/4 Platforms. A bar chart on the right shows the trend from 45 rows in 0.22.0 to 47 rows in 0.23.0.
```

For `docs/compare/graphics/2-against-the-editors.png`:

```alt
A dark feature matrix comparing Torqwave 0.23.0 and Torqwave 0.22.0 against Adobe Audition, Audacity, iZotope RX, Steinberg SpectraLayers and Steinberg WaveLab across editing, spectral repair, loudness, multitrack and plug-in rows, with green circles for yes, half yellow circles for partial, dashes for no and question marks for not confirmed.
```

For `docs/compare/graphics/3-against-the-daws-1.png` and `4-against-the-daws-2.png`:

```alt
Two dark feature matrices comparing Torqwave 0.23.0 and Torqwave 0.22.0 against ten DAWs (FL Studio, Ableton Live, Bitwig Studio, Reason, REAPER in part 1; Logic Pro, Cubase, Pro Tools, Fender Studio Pro, Ardour in part 2). Torqwave 0.23.0 turns green on Audio to MIDI and CLAP compared with dashes in 0.22.0, while still showing dashes for Dolby Atmos and ARA 2.
```
