# Plan 0.19.3: factory sequence presets, a basic drum machine, and a library that sorts by style

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

Steps 1 to 7 are done. Using them showed that the built-in voices aren't good enough. Basic Synth sounds thin and cheap. Basic Drums played only 22 of the 47 General MIDI drums, so many of the Sequencer's drum rows were silent. Both clicked: when a note was cut off, struck again, or stopped by the transport, and on short attacks. The user's call (2026-11-21): the built-in voices should sound good enough to keep, not just to preview. Basic Synth is replaced in place by a synth in the spirit of a lighter Massive. Older sessions using it will sound different, by the user's choice. Basic Drums gets better sounds. 0.19.3 waits for all of it, with about 1,000 presets written for the new voices.

### Step 8: no clicks (S, partly done)

- **Basic Drums:**
  - Every General MIDI drum from 35 to 81 now has a sound. **Done.**
  - A sound struck again, or a hat choked, fades out over 3 ms instead of stopping dead. **Done.** The test measures the jump where a ringing conga is struck again: 0.038 a sample with the fade, 0.322 without.
- **Basic Synth, the same rules, carried into the new engine:**
  - A note struck again while it sounds keeps its wave's phase and its filter state.
  - A stolen voice, and All Sound Off from a stop, a loop or a seek, fade over 3 to 5 ms.
  - The attack is never shorter than about 1.5 ms.
  - Today, each of these restarts the wave or cuts it at full level. That is the pop the user heard on every note of Falling Note Ramp, at the end of Minor Bell Motif's loop, and when Low Boom plays.
- **Tested:** a click check that renders every factory preset through its voice, as a loop, stopped and restarted. It fails when a sample jumps further than the sound itself can move.

### Step 9: Basic Synth becomes a real synth (L, the largest piece)

The aim is a lighter Massive: rich, wide sounds from a few panels. It is not a copy of Massive's look or names.

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

### Step 12: the presets again, for the new voices (L)

- About 1,000 sequences and about 100 kits across 36 styles, including game audio's UI sounds and stingers. The generator for them is in `Scripts/make_presets.py`.
- Each preset plays on a named synth patch or drum kit from steps 9 and 10.
- Each preset keeps the step 6 rules, peaks between -24 and -1 dB, and passes the click check.
- The guide's Styles page grows to cover all 36 styles.

### Step 13: Swarm Guitar (M)

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
