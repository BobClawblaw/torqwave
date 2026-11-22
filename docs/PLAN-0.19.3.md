# Plan 0.19.3: factory sequence presets, a basic drum machine, and a library that sorts by style

This plan was released as **0.20.0**: it grew past a point release (a new synth, a new drum machine, StringSwarm, Remap Notes and 1,018 presets). The steps keep the number they were written under.

Torqwave's Sequence Library starts empty, so a new user has nothing to drop, hear or learn from. Its drum sequences can only play on Basic Synth, which makes tones, not drums. And the library sorts by role alone (Drums, Bass, Harmony and so on), with no way to ask for "dark techno" or "EBM".

This plan adds three things:

- **Factory presets:** at least 20 sequences in every role, written for dark techno and industrial/EBM first and for the techno styles around them.
- **Two basic built-in voices to hear them:** a basic drum machine, and a few additions to Basic Synth.
- **A library that groups and filters by style, energy and character**, not just by role.

**The presets are notes for your own instruments.** They are mostly meant to drive VST3 and AU instruments, so they are written as clean MIDI:

- drums on the General MIDI drum map;
- bass in the octaves a bass synth expects;
- velocities and note lengths that carry the groove.

A preset dropped on a track with an instrument plays on that instrument. The built-in voices are only for previewing, and for a track that has no instrument yet. They stay deliberately basic.

Each piece below has:

- what it is;
- what it builds on in the code today;
- its size: **S** is days, **M** about a week, **L** more;
- how it is tested.

Each step lands as its own commit with its tests, guide pages and a changelog entry that says what was not tried by hand. Problems found by use go into the [fix log](DOGFOODING.md) with an issue, as each is fixed.

## What the code gives us to build on

- **The Sequence Library** (`Source/SequenceLibrary.h`). It is a folder of `.tqseq` files under the fixed roles and sub-roles:
  - Drums: Full Grooves, Kick, Snare & Clap, Hats & Tops, Percussion, Fills, Breaks.
  - Bass: Sub, Synth Bass, Arp Bass, Picked & Played.
  - Harmony: Progressions, Keys & Comping, Stabs, Strums.
  - Pads: Sustained, Evolving, Swells, Drones.
  - Arps: Up & Down, 16th Sequences, Plucks, Gated.
  - Melody: Leads, Hooks, Counter-lines, Bells & Mallets.
  - SFX: Risers, Downlifters, Impacts, Sweeps, Transitions, Hits.
  - Kits and Inbox.

  Each entry carries a name, key, tags, bars, rating, favourite mark and the time it was added. The panel sorts by Name, Bars, Rating or Newest, filters by search and Favourites, and has Fits the Session (same key).
- **Sequence files** (`SequenceFile`): notes in ticks (960 a beat), a written tempo, a key, a role and sub-role, tags, a note, and a voice: the instrument and effects of the track the sequence came from. A voice is optional.
- **Drops** (`Sequences`):
  - on empty space or an audio track, a sequence makes a new instrument track from its voice;
  - on an instrument track, it adds the notes only and keeps the track's instrument;
  - with Option, it makes a new track.

  This is the behaviour presets need, already.
- **Preview** (`SequencePreview`): a click in the library plays a sequence through its own voice at the session's tempo.
- **Basic Synth** (`BuiltInEffects.h`, `BasicSynthProcessor`):
  - one oscillator (sine, triangle, saw, square);
  - a fixed low-pass filter (cutoff, resonance);
  - an ADSR;
  - pitch bend, the mod wheel and a few controllers.

  It has no filter envelope, no glide and no noise, so it can't play drums, and its basses and acid lines sound flat.
- **The drum grid** (`NoteEditor`) shows drums by the General MIDI map unless a track has its own map.
- **Kits** (`.tqkit`): several sequences saved as one, dropped as a folder of tracks.

## Step 1: Basic Drums, a built-in drum machine (M)

A built-in instrument, **Basic Drums**, that plays the General MIDI drum map with simple synthesised sounds. It is enough to hear a pattern, and not meant to replace a drum plug-in.

**Sounds.** Each is a few lines of DSP, all on the note numbers that drum plug-ins and kits use:

| Note | Sound | How it's made |
|---|---|---|
| 35, 36 | Kick | sine with a pitch drop and a click |
| 37 | Rim | short filtered click |
| 38, 40 | Snare | tone plus noise |
| 39 | Clap | three noise bursts and a tail |
| 42, 44 | Closed hat, pedal hat | high-passed noise and metallic squares |
| 46 | Open hat | as the closed hat, longer, choked by a closed hat |
| 41, 43, 45, 47, 48, 50 | Toms | tuned sines with a pitch drop |
| 49, 57 | Crash | long noise |
| 51 | Ride | metallic squares with a bell |
| 56 | Cowbell | two squares |
| 70 | Shaker | short noise |

- **Settings:** Level, and Tune and Decay for kick, snare, hats and toms. Velocity sets the level. A hat choke group.
- **Where it's used:** it becomes the voice of new drum sequences, and the preview voice for drum presets when no drum plug-in is chosen (step 5).
- **Tests:** every mapped note makes sound and an unmapped one makes none. Velocity scales the level. An open hat stops at the next closed hat. Each sound ends within its decay. It renders the same twice and with any block size. It saves and restores its settings with the session.

## Step 2: Basic Synth, enough for bass and acid (S)

Four small additions to Basic Synth, each off by default, so every existing session and sequence sounds exactly as before (tested bit for bit):

- **Filter envelope:** an amount and a decay. This gives the plucked bass, stab and acid sound the presets are written for.
- **Glide and legato:** with Glide above 0, a note that starts before the last one ends slides to it without restarting its envelope. This is the 303 slide; the acid presets mark a slide by overlapping the next note.
- **Velocity to filter:** loud notes open the filter further. This is the 303 accent.
- **Three oscillators** (user, 2026-11-21): oscillators 2 and 3, each with its own wave, octave (-2 to +2), detune (±50 cents) and level, off (level at -inf) by default. Stacked and detuned they make the fat bass, stab and lead sounds; an octave down makes a sub.

No noise and no LFO beyond the mod wheel. Those belong to the user's own plug-ins.

**Tests:** at the old defaults, renders are bit-identical to 0.19.2. The filter envelope's decay shows in the spectrum. A legato note keeps its envelope and glides in the time set. Velocity changes the brightness only when it's switched on. Oscillators 2 and 3 add their partials at their octave and detune, and at their default levels add nothing.

## Step 3: the Presets section (M)

Factory presets ship with the app, read-only, and appear in the Sequence Library under a **Presets** heading of their own, above the user's library, with the same roles and sub-roles. They are never written to the user's folder:

- Using one (drop, double-click, Idea Board, Ask Torq) reads it from the app.
- **Edit in Sequencer** opens it as a new, unsaved copy, and **Save** puts that copy in the user's library under the same role, sub-role and name.
- A later version can replace the presets without touching anything the user made.

**Voices.** A preset has no instrument of its own. Instead it names a **preview voice**: Basic Drums for drums; Basic Synth with a setting (bass, stab, pad, lead, pluck, acid) for the rest.

| Where it's dropped | What it plays on |
|---|---|
| A track with an instrument | that instrument (as drops do now) |
| Empty space or an audio track | a new track with the preview voice |
| The Sequencer | the preview voice, which the user can change in the Voice panel as now |

**A Presets button in the Sequencer.** Its menu lists the presets by role and style, and opens the chosen one there as a new copy, the same as Edit in Sequencer from the library.

**Tests:**

- The Presets heading lists every factory preset under its role and sub-role.
- Nothing is written to the user's folder until Save.
- A preset saved from the Sequencer lands in the user's library with its notes and settings.
- A drop on an instrument track keeps the track's instrument; on empty space it makes a track with the preview voice.
- Presets are found by search, Favourites and the new filters (step 4).

## Step 4: the library sorts and groups by style (M)

New things a sequence (factory or the user's) can carry. All are optional, and older files read as before:

- **Style:** a genre and a style, such as Techno › Dark or Industrial › EBM. Step 6 lists the styles the presets use, and the user can type their own.
- **Energy:** 1 to 5, from intro tool to peak.
- **Character:** pairs that are quick to tick and hard to get wrong. Dark/Bright, Sparse/Dense, Straight/Swung and Clean/Distorted.
- **Description:** one line on what it does, such as "16ths between the kicks, octave up on beat 4". It shows under the name and doubles as a lesson.

**In the panel:**

- **Group by:** Role (as now), Style, Key, or Kit. Grouping by style shows Techno › Dark › Bass, Drums, and so on.
- **Filters:** a Style list, Energy, Character, Bars (1, 2, 4, 8) and Key. Factory and Mine show the presets, the user's own, or both.
- **Sort by:** Name, Bars, Rating, Newest (as now), plus Tempo, Key and Energy.
- **Saved searches:** a filter set saved by name, shown with the folders and kept up to date.
- **Similar:** on a sequence's menu, shows those of the same role and style with the nearest energy and the same key.
- **Transpose to the session's key** when dropping. It is a setting, off by default; Fits the Session still filters by key.
- **Sequence Properties:** a dialog to set all of these on a user's sequence. It is also used when saving from the Sequencer and adding to the library.

**Tests:** each filter, group and sort on a fixed set of entries. Older files read with none of these and are written back unchanged. Saved searches come back from the settings. Transposing a drop moves every note by the interval between the keys and leaves drums alone.

## Step 5: Preview on your own instruments (S)

A **Preview on** choice in the Sequence Library:

- **Built-in:** the preview voices.
- **The selected track's instrument:** a drum preset previews through whatever drum plug-in the selected track holds.
- **A chosen plug-in for each role:** for example a drum plug-in for Drums and a bass synth for Bass. The choice is remembered.

Preview stays as it is now otherwise: in time with the session, at its tempo, looping.

**Tests:**

- With a plug-in chosen for a role, a preview of that role renders through it, using the built-in effects as the stand-in for plug-ins in tests.
- With the track's instrument chosen, it follows the selection.
- The choice comes back from the settings.

## Step 6: the presets themselves (L)

**Generated, not hand-entered.** The presets are generated by a script in the repository (`Scripts/make_presets.py`). It writes them from compact pattern definitions, such as a 16-step grid per drum, a step list for a bass line or a chord progression, plus their variations. The `.tqseq` files are built from it, so a change to the conventions is one edit and a rerun.

**Rules every preset follows** (checked by the tests):

- **On the grid.** Notes start on 16ths, and 8th or 16th triplets only in presets tagged Swung or Triplet. Swing is a separate variant, not baked into the straight one.
- **Drums use the General MIDI map:** kick 36, rim 37, snare 38, clap 39, closed hat 42, open hat 46, ride 51, toms 41/45/48, crash 49, shaker 70.
- **Bass lines are mono:** no overlapping notes, except an acid slide, which overlaps the next note by 20 ticks.
- **Velocities carry the groove:**

  | Hit | Velocity |
  |---|---|
  | Accent | 110–127 |
  | Normal | 90–105 |
  | Weak hat | 60–80 |
  | Ghost | 25–50 |

- **One home key: A minor**, with Phrygian (B♭ in place of B) and harmonic minor (G♯ in place of G) where a style wants them. A1 is 55 Hz, which sits well in the sub, and every preset stacks with every other. Transpose on drop (step 4) moves them to any key.
- **Lengths of 1, 2, 4 or 8 bars.** A 4-bar preset changes in bar 4: a fill, an answer, or a root move.
- **Written tempo** is the style's usual one. Sequences follow the session's tempo anyway.
- **Original patterns only.** Each one is written to a style's conventions, never copied from a song. Names describe the style ("Belgian Body Bass", "Rumble Roller"), never a band or a song.
- **Each has a one-line description.**

**The styles** come from the research (sources in the commit; the main ones are Attack Magazine's *Beat Dissected* series, Native Instruments' and Beatportal's melodic techno guides, Sound On Sound, and MusicRadar on the TB-303).

**Weighting.** The weighting follows the music this is for. Most of it is driving and peak-time techno in the German and Spanish line: big rolling basses, percussive loop tracks and stabs at 130–140 BPM (the Sven Wittekind, Torsten Kanzler, Hackler & Kuch, Niereich, UMEK, Maksim Dark, Oliver Huntemann, Spektre and Stigmata kind of record). After that:

- dark and industrial techno (Ancient Methods, Ansome, SHDW & Obscure Shape, Maenad Veyl, Violet Poison);
- hypnotic techno (the Non Series: Luigi Tozzi, Biri, IORI);
- on the band side, EBM and electro (Front 242, And One, Severed Heads) and today's darkwave (Rosa Anschütz, Pixel Grip).

So driving and peak-time techno, dark and industrial techno, and EBM carry the most presets. Melodic and minimal techno get a smaller set.

| Genre › Style | Tempo | What defines it |
|---|---|---|
| Techno › Dark | 128–135 | Short kick plus a long rumble on the 16ths after it, offbeat hats under 70% velocity, ride in 8ths with every second hit quieter, root-note rolling bass, one-note minor stabs |
| Techno › Driving / Peak-time | 130–138 | Kick-bass-bass-bass rolling 16ths, clap on 2 and 4 layered with a snare, velocity-shaped 16th hats, ghost kicks before 2 and 4, call-and-response riffs |
| Techno › Percussive / tool | 132–142 | Loop tracks built from percussion: tom, conga and metal figures in 16ths with shifting accents, a rolling kick-bass, little or no harmony; the German driving and Schranz-adjacent sound |
| Techno › Hypnotic | 128–134 | Polymetric 3- and 5-step percussion against the bar, very few pitches, 4- and 8-bar loops that change slowly |
| Techno › Minimal | 122–130 | Sparse clicks and rims, short syncopated bass, one dub chord, swing at 54–60% |
| Techno › Club tools | 125–132 | Kick only, kick and hat, top loops, percussion loops: parts built for layering |
| Techno › Melodic | 120–126 | Rolling 16th bass on the chord roots, i–♭VI–♭III–♭VII and i–iv–♭VI–♭VII progressions, 16th arps over the chords, 1-bar leads restated over 8 |
| Techno › Industrial | 126–130 | Overdriven kick, low tom figures in the kick's gaps, noise snare, metallic percussion |
| Techno › Hard | 145–155 | Offbeat rumble, metallic 16th percussion with 3- and 6-step accents, kick rolls |
| Techno › Acid | 128–140 | 303 lines: 16 steps, root with octave jumps, rests, 25–40% accents, 10–25% slides, minor pentatonic or Phrygian |
| Industrial › EBM | 110–130 | Sequenced 16th or 8th bass in the kick's gaps, the low-to-octave jump, short gate, rests so it breathes; root moves of i to ♭VI to ♭VII or i to ♭II; disco or backbeat drums with near-flat "machine" velocities and big gated snares; alarm lines a semitone or tritone apart |
| Industrial › Electro-industrial | 100–135 | Syncopated kicks, flammed snares, 16th bass with a semitone shift, Phrygian and harmonic minor clusters, pitch-bend dives |
| Industrial › Industrial rock | 90–140 | Programmed rock beats, synth riffs doubling guitar in 8ths with ♭2 and ♭5 moves, power chords |
| Industrial › Futurepop | 125–140 | Trance drums and offbeat bass, minor 4-chord loops, gated 16th arps, 2-bar call and answer leads |
| Industrial › Aggrotech | 130–150 | Hard 4/4, snare-roll fills, distorted offbeat bass, 16th supersaw riffs |
| Industrial › Synth-EBM | 120–135 | EBM bass sequences under synth-pop chords and a sung hook: 8th and 16th bass on the roots, minor progressions, square and saw leads in 2-bar phrases |
| Industrial › Darkwave | 100–118 | Drum-machine pop beats with long-reverb snares, 8th root bass, chorused minor pads, simple 2-bar melodies |
| Industrial › Techno-body | 125–140 | Techno kick and hats with an EBM 16th bass in the gaps, metallic percussion, root changes on bar 3 or 4 |

A note on history: Front 242 made "Electronic Body Music" known with their 1984 *No Comment* EP. Earlier uses exist, Ralf Hütter's in 1977–78 and DAF's "Körpermusik" in 1981, so the guide will say Front 242 *popularised* the term.

**How many.** At least 20 in each of the seven roles, 140 or more in all, weighted towards dark techno and EBM. Roughly:

| Role | Count | What's in it |
|---|---|---|
| **Drums** (30) | 30 | Full grooves per style; kick and kick-rumble tools; top loops; percussion and polymetric loops; fills; builds (16th snare and clap rolls ramping from 40 to 127) |
| **Bass** (30) | 30 | EBM gallops and octave sequences; rolling techno; offbeat; acid; rumble; melodic-techno root lines; darkwave roots |
| **Harmony** (20) | 20 | Minor and Phrygian stabs; dub chords; melodic and futurepop progressions; power-chord riffs |
| **Pads** (20) | 20 | Drones; root-and-fifth swells; slow minor progressions; harmonic-minor clusters |
| **Arps** (20) | 20 | Gated 16ths; up-and-down; 1-5-8-♭3 broken figures; plucks; acid-style arps |
| **Melody** (20) | 20 | Short motifs; alarm and siren lines; hooks with call and answer; counter-lines |
| **SFX** (20) | 20 | Risers and downlifters made from note ramps and pitch-bend lanes; snare and tom builds; impacts; sirens |

**Starter kits.** One for each of the eight main styles, in Kits. Each holds a drum groove, a bass, a stab or chord, a lead or arp, a fill and a build, all in the same key, tempo and length, so stacking one of each makes a track.

**Tests (the script's output, checked by the suite):**

- every role has at least 20 presets;
- names are unique;
- every note is on the grid, in range and in the preset's key or scale, apart from drums and marked chromatic notes;
- bass lines are mono except marked slides;
- drums use only mapped notes;
- lengths match their bars, and descriptions and styles are set;
- every kit's parts agree on key and tempo;
- running the script again produces identical files.

## Last: the guide, screenshots and the changelog

- A guide section, "Start from a preset", in the Sequencer chapter.
- The Basic Drums and Basic Synth settings documented.
- A short "Styles" page with the table above, so the presets teach.
- Screenshots of the Presets section and a style kit in the Sequencer, every note on the grid.
- The changelog entry, and the release as 0.19.3.

## Order and size

1. Basic Drums (M)
2. Basic Synth additions (S)
3. The Presets section and preview voices (M)
4. Styles, filters and grouping in the library (M)
5. Preview on your own instruments (S)
6. The presets and kits (L)
7. Guide, screenshots, release

Steps 1 and 2 can run beside each other; they touch different code. Step 6's script can be written beside steps 3 to 5, but its tests need step 4's file fields.

That is more than a patch usually holds. If 0.19.3 should ship sooner, steps 1, 2, 3 and 6 (the drum machine, the synth additions and the presets) can go first, and steps 4 and 5 can follow as 0.19.4.

## Part two: sounds worth keeping

Steps 1 to 7 are done. Using them showed that the built-in voices aren't good enough. Basic Synth sounds thin and cheap. Basic Drums played only 22 of the 47 General MIDI drums, so many of the Sequencer's drum rows were silent. Both clicked: when a note was cut off, struck again, or stopped by the transport, and on short attacks. The user's call (2026-11-21): the built-in voices should sound good enough to keep, not just to preview. Basic Synth is replaced in place by a full wavetable synth of Torqwave's own (Torqwave Synth). Older sessions using it will sound different, by the user's choice. Basic Drums gets better sounds. 0.19.3 waits for all of it, with about 1,000 presets written for the new voices.

### Step 8: no clicks (S, done)

- **Basic Drums:**
  - Every General MIDI drum from 35 to 81 now has a sound. **Done.**
  - A sound struck again, or a hat choked, fades out over 3 ms instead of stopping dead. **Done.** The test measures the jump where a ringing conga is struck again: 0.038 a sample with the fade, 0.322 without.
- **Basic Synth, the same rules, carried into the new engine:**
  - A note struck again while it sounds keeps its wave's phase and its filter state.
  - A stolen voice, and All Sound Off from a stop, a loop or a seek, fade over 3 to 5 ms.
  - The attack is never shorter than about 1.5 ms.
  - Today, each of these restarts the wave or cuts it at full level. That is the pop the user heard on every note of Falling Note Ramp, at the end of Minor Bell Motif's loop, and when Low Boom plays.
  - **Done** in the new engine (step 9): a stolen voice and All Sound Off fade over 4 ms, the amp takes at least 1.5 ms to rise or fall and 3 ms to release, and a voice ends at exactly nothing (its DC blocker sits before the amp, so no tail is cut). The click test renders Falling Note Ramp, Minor Bell Motif and Low Boom on their voices plus a retrigger, steal, release-in-the-attack and All Sound Off case: the largest step between samples is 0.081 (the bell's own sine), under the limit of 0.15; 0.19.2's synth makes 0.29, 0.25 and 0.27 there.
- **Tested:** a click check that renders every factory preset through its voice, as a loop, stopped and restarted. It fails when a sample jumps further than the sound itself can move.

### Step 9: Basic Synth becomes a real synth (L, the largest piece, done)

The aim when this step was written was a lighter take on a classic wavetable synth: rich, wide sounds from a few panels, in its own look and names. It has since grown well past "lighter": by 0.20.0 it is a full synth in its own right (parts two and three), and is no longer described as a lite version of anything.

**As built:** a synth of its own, in its own look and words, that takes up the best ideas of the classic wavetable synths: three wavetable oscillators with Position, serial or parallel filters, macros and a step LFO; a warp mode on each oscillator, a sub and a noise oscillator, a large wavetable view, drag-to-modulate rings and an effects rack. Added at the user's request: a plucked **String** table (each unison voice a string tuned to the note, with Strum and Drift on unison), and Fuzz in the drive, for the fuzzed string sounds. The factory patches are all for dark techno, industrial and EBM, with no keys or brass: 97 made by hand (Init among them), from basses and reeses to impacts and textures, and, at the user's later request for at least a thousand, a bank of 970 more drawn from 24 kinds of sound, with two more categories (Perc and Vox): 1,067 in all. At the user's request it is renamed **Torqwave Synth** (the plug-in ID stays `torqwave:basic-synth`), and the lead first called StringSwarm is **Wailing Fuzz Lead**, so it doesn't share a name with the built-in StringSwarm instrument.

- **Three wavetable oscillators:**
  - Each has a table and a **Position** that morphs through it.
  - The tables: analogue (sine, triangle, saw and square shapes), PWM, formant or vowel, digital or FM, metallic, and a sub.
  - Each table is band-limited for every octave, so high notes don't alias.
  - Each oscillator has pitch, fine tune and level, plus a noise source.
- **Unison:** up to 8 voices an oscillator, with detune and stereo spread. The synth becomes stereo; today it is mono.
- **Two filters**, in series or in parallel, each with drive and key tracking:
  - a 24 dB ladder low-pass, for warm, resonant bass and acid;
  - a state-variable filter: low, band and high pass, and notch.
- **Modulation:**
  - Four envelopes: one for the amp, three free.
  - Two LFOs, with shapes, tempo sync and retrigger.
  - Eight modulation slots, each a source, a destination and an amount.
  - The sources: envelopes, LFOs, velocity, the mod wheel, aftertouch and the key.
  - The destinations: nearly any knob.
  - Four **macros** for presets and automation.
- **Voices:** poly, mono or legato with glide (the 303 slide stays), and the declicking from step 8.
- **Effects inside the synth:** drive or saturation, chorus or ensemble, a tempo-synced delay and a reverb, each with a mix.
- **Its own window** instead of JUCE's parameter list, in Torqwave's look:
  - the oscillators, the filters, the envelopes and LFOs, and the effects and modulation slots;
  - the waveform drawn at its position;
  - a patch browser.
- **Factory patches:** about 100 named sounds (bass, reese, acid, lead, pad, pluck, keys, bell, brass, FX). The presets use them by name.
- **Old sessions:** the old parameter IDs load into the new engine as the nearest equivalent sound. Sessions load, but don't sound bit for bit as before. The changelog says so.
- **Tested:**
  - each table measured for aliasing;
  - unison measured for width;
  - the ladder filter's resonance and cutoff measured against their settings;
  - every modulation route shown to move its target;
  - renders the same at any block size;
  - CPU at 16 voices with 8-voice unison, measured on the Mac;
  - the click check from step 8;
  - old sessions loading.

### Step 9b: a second bank of patches (M, done)

About 1,500 more factory patches, asked for after step 9 shipped with 1,067. Step 9 kept the factory patches to dark techno, industrial and EBM, with no keys or brass; this step keeps those first and adds what sound design, picture and game work reach for.

- **1,493 patches** in `Source/WaveSynthPatchBank2.cpp`, making 2,560. More in the categories there were, and thirteen new ones: Sub, Chord, Arp, Keys, Organ, Brass, Strings, Cinematic, Drum, FX, Glitch, Game and Chip.
- **Made by measurement.** `Scripts/make_patch_bank.py` draws each of 193 kinds of sound (`Scripts/patch_bank_kinds.py`) four times for every patch wanted, plays every candidate through the synth and keeps a patch only if it sounds in its register, does not click, has four macros that are heard, and is not a near-copy of a patch already in its category. The file it writes is plain settings, the same on every machine.
- **The tests hold the bank to the same things** (`Tests/WaveSynthBankTests.cpp`), and `Scripts/patch_settings.txt` lists every patch with a hash of its settings, so a patch that changes shows.
- **Existing patches are untouched**: they keep their names, settings and places, and a category's new patches come after its old ones, so the sequence presets pick the same voices.
- **Tried and dropped:** a lead and a riser made from a filter at the edge of oscillating (the ladder's peak sits about a semitone under the key, and it hisses more than it whistles); band-pass filters rung by a click as percussion (40 dB too quiet; those sounds are made from sine partials instead); an overdriven acid kind (every candidate was too near the first bank's). Not attempted: an acoustic piano and solo bowed strings, which the engine has no way to make; Guitar, which another piece of work is changing.
- **Not done:** nobody has listened to them. Everything above is measured.

### Step 10: Basic Drums sounds good (M to L)

- **Better synthesis for each sound:**
  - The kick: a two-stage pitch envelope, a click layer and drive. A choice of three characters: boom, punch and hard.
  - The snare: two tuned body modes and filtered noise with its own transient.
  - The clap: more bursts and a short room.
  - Hats and cymbals: six metallic oscillators through tuned band-pass and high-pass filters. Cymbals are ring-modulated, with longer noise.
  - Toms and hand drums: pitch drops and a strike.
- **Every sound:** tune, decay, level, pan and tone. Choke groups (hats, triangle and cuica). Stereo out.
- **Kit-wide controls:** drive, a glue compressor and the level.
- **Several kits from the same engine:** dark techno, industrial, EBM machine, electro, breaks and acoustic-ish. Each sequence preset names its kit.
- **Its own window:** a pad for each sound, click to hear it, with that sound's settings beside it.
- **Tested:**
  - all 47 notes sound, none clipping;
  - each sound's settings change it;
  - chokes, renders identical at any block size, and the click check.

### Step 11: drum patterns on any drum plug-in (M)

Dropped on a track with a drum plug-in (Battery, for one), a drum preset plays its General MIDI notes. That is right for a kit laid out on General MIDI, and wrong for one that isn't. Today the track's drum map only renames the rows; it doesn't move notes.

- **Remap Notes**, on the drum map, moves the clip's notes from General MIDI to the kit's own layout (snare 38 to the kit's snare key, and so on). It is one undo step.
- **Maps are saved by name:** for example, "Battery: Dark Kit". A map is chosen per track, and every drum preset dropped on that track is remapped as it lands.
- **Learn:** pick a General MIDI sound, then hit the matching pad on the plug-in or a MIDI controller.
- **Tested:** a preset dropped on a track with a saved map lands on the map's notes. Remap Notes and its undo give the notes back exactly.
- **Done**, as planned except that the map is the track's drum map itself (each sound saying which General MIDI drum it stands for) and Learn hears MIDI inputs only: a pad clicked in a plug-in's window sends no note to the host. Not yet tried by hand in the running app.

### Step 12: the presets again, for the new voices (L)

- About 1,000 sequences and about 100 kits across 36 styles, including game audio's UI sounds and stingers. The generator for them is in `Scripts/make_presets.py`.
- Each preset plays on a named synth patch or drum kit from steps 9 and 10.
- Each preset keeps the step 6 rules, peaks between -24 and -1 dB, and passes the click check.
- The guide's Styles page grows to cover all 36 styles.
- **Done:** 1,018 sequences and 116 kits, each on a named patch or kit, levels calibrated from rendered peaks. The click check runs on every preset, played once through its voice (not looped, stopped and restarted, which the synth's own click tests cover). The Styles page covers all 37 styles.

### Step 13: StringSwarm (M)

A guitar with every string tuned to the played note, through a fuzz, as a third built-in instrument (`Source/SwarmGuitar.*`), only in Torqwave: no plug-in of it for other hosts for now.

- **Done:** the engine (modelled strings, tunings, detune, strum and Strum Rate, layers each through its own amp, fuzz, crunch and cabinet, no clicks), its 15 hosted parameters, its window from the shared control kit (`Source/SwarmGuitarEditor.*`: knobs with value arcs, the strings drawn ringing, the tuning shown string by string, the amp's curve, the guitars across the field) and its tests.
- **To do:** factory patches, a guide screenshot, and presets that play on it in step 12.

### Order

1. Step 8's synth half, inside step 9's voice code.
2. Steps 9 and 10. They can run side by side, since they touch different code.
3. Step 11.
4. Step 12, last, because it needs the new patches and kits.
5. Step 13's window after step 9's control kit lands.

The Levels panel standing upright when docked at a side (a fix from use) and double-clicking sequences and kits in the Finder go in with them.

## Part three: sounds from sounds

Added on 2026-11-22, for the same release (0.20.0). Everything here is inside Torqwave Synth: no new instrument.

**It is our own design.** Other synths find patches by variation and from recordings; none of their ideas for showing it are used here. No seeds, plants, DNA, genes, breeding, parents or children, in the look or in the words. Ours is a **signal console**: futuristic, cyber, a hacker's map of a network of sounds. A patch is a **node**; the sounds near it are nodes it can reach; moving through them leaves a **route**; a recording is a **signal to crack**.

Two things are still to be decided, and the steps say where: the names (written **Trace** for the map and **Decrypt** for matching until they are chosen), and whether a trained model gives the search its first guess (step 17 is search alone, which needs no model).

### Step 14: groundwork in the synth (M)

Three additions that matter more to reproducing a sound than more oscillators would. The oscillators stay at three: each already has up to 8 unison voices, and seven more would add about 80 settings for a search to wander in, at three times the CPU.

- **A wavetable from audio.** A new table type, "Sampled", whose frames are made from a clip: its spectrum taken at steps through the sound, each written as one band-limited frame, so Position plays through the clip's tone from start to end. Made by dropping a clip on an oscillator, or by Decrypt (step 17). The frames are saved in the patch's state, so a patch with one plays anywhere. 64 frames at most.
- **A level envelope for each oscillator.** Attack, hold, decay and sustain on the oscillator's level, off by default (level as now), so layers come and go on their own without spending modulation slots.
- **16 modulation slots** in place of 8. Patches and sessions saved with 8 load as they are.
- Nothing changes for patches that use none of this: every factory patch renders the same as before, checked sample for sample.
- **Tested:** a sine swept from 200 Hz to 2 kHz, made into a table, plays back within 1 dB of the clip's spectrum at five positions; a table saved in a patch comes back the same; an oscillator with its level envelope at 200 ms attack reaches half level when the envelope says it should, while the amp envelope is open; slot 16 modulates as slot 1 does; every factory patch renders the same as before this step; no clicks (the step 8 check) with a Sampled table and a level envelope retriggered.

### Step 15: Trace, a map of the sounds within reach (M)

**Changed on 2026-11-22, after the first version was used by hand:** Trace is not a feature of its own in 0.20.0. Its mechanics were clear in use, but twelve unsteered variations gave no reason to use it over the knobs or the patch browser. The scope stays as **Decrypt's screen** (step 17): it shows the candidates of a search, their lock, and lets one be heard and taken. What follows in this step is kept as the record of what was built; of it, the engine (nodes, levelling, addresses, the route) and the scope's drawing are used by Decrypt, and the rest is not offered in the window: no KNOBS | TRACE switch for exploring, no Reach, firewalls, Bridge, Rescan or nodes on keys. An idea kept for a later release: the twelve as named pushes in musical directions (brighter and darker, punchier and softer, thicker and thinner, more and less motion, dirtier and cleaner, longer and shorter).

- **The Trace view** in the synth's window, beside the knobs. In the middle the node you hold: the patch as it is. Around it twelve nodes within reach, each the patch with some of its settings moved. Click a node to hear it, double-click to **jump** to it, and the map redraws round where you are now.
- **The route** is where you have been: a line of the nodes you jumped through, along the bottom. Click any of them to go back there. Nothing is lost by exploring.
- **Reach** sets how far away the twelve nodes are: near (small changes) to far (a different sound).
- **Firewalls** keep a part of the patch as it is while you move: oscillators, filters, envelopes, modulation or effects. A firewalled part is the same in every node.
- **Bridge** links to a second patch (any factory or saved patch) and lays the twelve nodes along the line between the two.
- **Nodes on keys:** with the view open, the octave from C3 plays the twelve nodes, one a key, so a keyboard or a clip auditions them; the other keys play the node you hold.
- **The reach map:** a table in the code of which settings move, how far, and which firewall holds them. Choices (a table, a filter type) change rarely; levels never move so far that a node is silent or clips: each node is rendered for a moment and levelled to the one you hold.
- Every node is an ordinary patch: jumping to it sets the knobs, and it saves as a preset like any other.
- **Tested:** the same node, Reach and seed give the same twelve nodes; a firewalled part's settings are unchanged in every node; at Reach 0 every node is the one you hold; every node round 50 factory patches sounds (peaks above -40 dB) and none clips; a click on the route returns those settings exactly; C3 to B3 play nodes 1 to 12 and C4 the node you hold.

### Step 16: the console's look (M to L: this is the part people will remember)

The look is the feature's identity, so it gets its own step and a mock-up the user sees before it is built.

- **A network map, not a menu.** The node you hold glows in the middle; links run out to the twelve within reach; the route trails behind as a dimmer line. Jumping animates along the link, and the map re-forms round the new node (under 250 ms, and not at all when the system's Reduce Motion is on).
- **Each node is its sound's signature:** a small ring drawn from its spectrum over the first second (bright where it is bright, wide where it is loud), so nodes look as different as they sound. Under it a short hex address made from the patch's settings (the same patch always has the same address), in a monospaced face.
- **Console details that carry information, not decoration:** a scan line sweeps a node while it sounds; a firewall is drawn as a barrier across the links of the part it holds; Reach is a ring that widens round the middle node; the route shows each jump's address.
- **In the theme's colours**, with the accent as the glow, in light themes as well as dark (dark is where it looks its best, and the guide's pictures use it). Drawn with the control kit (`Source/SoundControls.*`), sharp at 70 % to 160 %.
- **Plain to use:** every control has an ordinary tooltip ("Reach: how far the nearby sounds are from this one"), so the vocabulary never has to be learnt to be used.
- **Chosen: Sweep** (2026-11-22), the third of three mock-ups (`docs/design/signal-console-mockups.html`, mock-up C; open it in a browser). It replaces the plain ring described above wherever the two differ:
  - **A scope.** Round the node you hold, five sectors: oscillators, filter, envelopes, modulation, effects. A node sits in the sector of the part that changed most, and its distance from the middle is how far it is from the node you hold: near rings small changes, far rings large ones. Reach sets how far out the twelve are spread. A sweep line turns slowly; it is the only motion at rest.
  - **A firewall closes its sector**: hatched, with a barrier arc at its inner edge, and no node lands there.
  - **The side panel** shows the node under the pointer: its signature large, its address and key, which part changed, and the three or four settings that moved most with their amounts in units ("amp attack -40 ms"); **Hear** and **Jump**; the route as a list beneath; and the line to type in when Ask Torq is on.
  - **In Decrypt the same scope reads as lock:** the signal's signature sits in the middle with the best candidate's laid over it, the rings are 75, 50 and 25 % lock, and candidates drift inward as theirs rises. The side panel shows the best lock as a large figure, a bar for each part as it settles, what the model proposed and whether it was kept, and **Stop** and **Take**.
  - The mock-up's colours stand for the theme's: the accent for nodes, a second colour for the signal, the theme's warning colour for firewalls.
- **Tested:** the view draws at 70 % to 160 % as the rest of the window does; a node's signature and address change when its settings do and are the same for the same settings; with Reduce Motion on, a jump redraws without animation; a guide screenshot of the real window.

### Step 17: Decrypt, a patch from a recording (L)

- **Drop a clip on the console** (or choose **Decrypt a Sound...**): one note, held or plucked, up to 4 seconds. Its pitch is found, it is trimmed to the note and levelled. Its signature is drawn in the middle as the target.
- **A search, on background threads:** candidates are rendered through the synth itself at the clip's pitch and length, and scored against the clip on its spectrum over time (on a mel scale, at three window sizes) and its loudness envelope. It starts from the factory patches nearest the clip, moves outward from the best, and repeats. The console shows the best twelve as its nodes, with a **lock** figure for each (how much of the signal that node accounts for), rising as the search goes; jump to one at any time.
- **How it looks while it works:** the target's signature in the middle with each candidate's laid over it, the parts that agree lighting up; the lock figures counting up; the patch's parts (oscillators, filter, envelopes) marked as each settles. It should feel like watching a signal being cracked, and every moving thing on it is a real number from the search.
- **With step 14's table:** the search may give oscillator 1 the clip's own Sampled table, and then has only the envelopes, filter and movement to find. This can be turned off (**Tables only from the factory**), for a patch made of the synth's own waves.
- **Dry in this version:** the effects are left off while matching, and a clip with heavy reverb or delay is matched as if it were dry. Chords and riffs are not matched: one note.
- **What a lock means,** said in the guide as it is: a sound the synth made is found again; a recording of something else gives a patch in its spirit, close for synth sounds, loose for voices, real drums and samples.
- No model is trained or shipped in this step. If the search's first guess needs to be faster, a network trained on the synth's own random patches can give it one later; that is a decision for after this is measured.
- **Tested:** ten factory patches of different kinds, each rendered and decrypted with their own names left out of the starting set, come back within a set spectral distance of the original (the distance is fixed when the search exists, from what a listener cannot tell apart in the renders); the same clip and seed give the same result; a search runs to its end without a dropout in a session that is playing (the audio thread's load measured before and during); the time it takes is measured on the development Mac and written in the guide; stopping it leaves the patch as it was.

### Step 18: the guide, the changelog and patches (S)

- The guide's synth section gains the Sampled table, the level envelopes, Trace and Decrypt, with screenshots of the real window, and says what a lock can and cannot do.
- A changelog entry for each step, with what was not tried by hand.

### Step 19: Ask Torq reaches all of it (M)

Ask Torq already chooses an instrument and a saved preset for a track (`set_instrument`, `list_instrument_presets`), sets its parameters, and writes sequences. It does not yet know the factory sounds or anything in part three. Each line below is a tool or an addition to one, with the plan step it writes shown to the user before Apply, in the one undo step where the session's state holds it.

- **Factory sounds by name and kind.** `list_instrument_presets` also lists Torqwave Synth's factory patches (by category, with a search word, since there are over a thousand), Basic Drums' kits and StringSwarm's presets; `set_instrument` takes one. "Put a dark reese on the bass track" picks from what is really there. **Done:** the list is 60 at a time with the categories and how many more; a name that matches nothing is refused with the five nearest.
- **Factory sequences and kits.** A tool to find factory sequence presets by role, style, energy and character, and one to drop a preset or a whole kit on the session at a bar. "Give me an EBM kit at 124" lands a real kit. **Done:** `find_sequence_presets`, `add_sequence_preset` and `add_kit`, through the code a drop runs. They reach the factory presets, not the user's own library.
- **Trace by words** is not built in 0.20.0 (see step 15); what follows it in this list stands. As it was planned: **Trace by words.** A tool that reads the node held and the twelve within reach as numbers (how bright, how loud over time, how noisy, how wide: the same measures the signatures are drawn from) and jumps to one, or sets Reach, a firewall or a Bridge. The model cannot hear; "darker, less attack" becomes a choice among measured nodes, and the plan says which measure it went by.
- **Decrypt from the session.** A tool that takes a clip in the session (or the selection in the editor), runs Decrypt on a track's synth, and reports the lock figure when it ends; the search runs in the background as it does from the window, and the result is one undo step.
- **A table from a clip.** A tool that gives an oscillator a Sampled table made from a clip.
- **Drum maps.** A tool that lists the saved drum maps, sets a track's map, and remaps a clip's notes to it or back to General MIDI. **Done:** `list_drum_maps`, `set_drum_map`, `remap_clip_notes`.
- **The new parameters** (the level envelopes, slots 9 to 16, StringSwarm's Swarm source and Strum Sync) are told to the model as the others are. **Done:** nothing had to change, and it is now tested. All 27 of StringSwarm's are read and set, Source and Strum Sync among them; all 235 of Torqwave Synth's are read, the level envelopes and slots 9 to 16 among them (listed 150 at a time, the rest reached with `find`). The Sequencer's tools name up to 256 of an instrument's parameters, so 235 fits; a synth that grows past 256 needs that limit raised.
- **What it says about limits:** Ask Torq's instructions carry the same plain statement as the guide: Decrypt matches one dry note, and a lock on a recording of something that is not a synth is loose.
- **Tested** (`--ai`, against a made-up model as the other tools are): each tool's plan step and its result; a patch asked for by a word that matches nothing is refused with the nearest names; a preset dropped by Ask Torq is the same as one dragged in; a jump made by Ask Torq equals a click on that node; Decrypt started by a tool can be stopped and leaves the patch as it was; Remap by a tool and its undo give the notes back exactly. With a real local model: three requests tried by hand and what happened written in the changelog. The factory sounds, sequences, kits and drum maps are tested with the made-up model. **With a real local model: done** for these, as an evaluation set rather than three requests by hand (`--ai-eval-set library`, 14 requests checked against the session): Qwen3.8-27B got 14 of 14 twice, with no wording changed. The results are in the changelog.

### Step 20: a local model helps, when Ask Torq is on (M, optional at every point)

Trace and Decrypt work in full with Ask Torq off. With it on, the model the user runs (the server chosen in Ask Torq's settings) is asked for help at four points. It never decides anything by itself: whatever it proposes is rendered and scored by the same measure as everything else, and kept only if it scores better. A slow or absent model costs nothing: each request has a time limit, and the search never waits for one.

- **Structure, which a search is poor at.** A search moves knobs well and makes choices badly (which table, which filter type, whether a second oscillator an octave up or a noise layer is needed). Between rounds of Decrypt, the model is given the target's measures, the best candidate's settings and where the two differ (which bands, which part of the note), and proposes up to five changes of structure. Each is tried; the search carries on from any that raise the lock. This is where a model is expected to help most, and it is measured: the same ten clips decrypted with and without it, lock and time written down.
- **Where to start.** From the target's measures (and the user's words, if they typed any: "it's a reese", "match the body, not the click"), the model picks categories and patches to start from and which parts to firewall, in addition to the nearest patches the search finds itself.
- **A model that hears, if the user has one.** If the server's model accepts audio, the clip itself is sent for a description, which joins the starting hints. If it does not, nothing is sent and nothing changes.
- **Names and notes.** A decrypted or traced patch is offered a name, a category, a line of description and names for its four macros, to accept or change. Without a model, it gets a plain name from its address ("Node 7F3A").
- **Trace by words** is step 19's tool; with a model on, the console has a line to type in ("darker", "more metallic, same envelope"), which calls it.
- **Private and local:** only the server already set in Ask Torq is used; a clip's audio goes to it only when its model takes audio and the user has turned on "Let the model hear clips" (off by default).
- **Tested** (with a made-up model): a proposal that scores worse is dropped and one that scores better is kept; a model that answers nonsense, late or not at all leaves the result the same as with Ask Torq off; nothing is sent when Ask Torq is off, and no audio is sent with "Let the model hear clips" off; the names offered can be refused. With a real local model: the ten-clip comparison above, and what it found, in the changelog.

### Order for part three

1. Step 14 first: 15 and 17 both write patches that may use it.
2. Step 16's look is chosen (Sweep).
3. Steps 15 and 16 together.
4. Step 17.
5. Step 19: the factory sounds, sequences, kits and drum maps can be done at once (they need nothing from part three); the Trace, Decrypt and table tools after steps 15 and 17.
6. Step 20 after step 17, since it is measured against it.
7. Step 18 as each lands.

Steps 14 to 17 all change `Source/WaveSynth*` and the synth's window, so they are one writer's work, one after another, after StringSwarm's update is merged.

## At release

- After 0.20.0 is on `main`: turn on GitHub Pages for the repository (source `main`, folder `/docs`), as the user asked on 2026-11-22, and check that the Sounds page (`docs/sounds/`) loads and plays there.
