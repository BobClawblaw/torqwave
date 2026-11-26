# Plan: Torqwave Synth Evolved

**Decided on 2026-11-25: not built as a separate instrument.** The user chose to put nine free oscillators into Torqwave Synth itself. What follows is kept as a record of what was weighed (the cost measured, the risks, the layers and scenes that were proposed); none of it is planned.

A scope, never started. Asked for on 2026-11-25: a nine-oscillator synth, "Torqwave Synth Evolved", as another instrument beside the ones there are.

## Where things stand

- **Torqwave Synth** has three wavetable oscillators, each with Position, a warp, up to eight unison voices and a level envelope of its own; a sub and noise; two filters in series or parallel; four envelopes, two LFOs, sixteen modulation slots, four macros; and drive, chorus, phaser, delay and reverb. Sixteen voices. 3,743 factory patches, Decrypt, favourites, Audition, and a table from any sound dropped on an oscillator.
- **What it costs today,** from its own test on the development Mac: sixteen voices in 8-voice unison through both filters and every effect take 5.9 % of one core with one oscillator and 9.3 % with all three, so about 1.7 % for each oscillator at that worst case.
- **Its rule:** nothing it does steps the sound, and it renders the same at any block size.

## Why another instrument, and not six more oscillators on this one

- **Everything saved rests on three.** Sessions, 3,743 patches, the user's presets and Decrypt's search all name three oscillators. Growing the synth in place puts all of them at risk for a change most patches would not use.
- **The window is full.** Three oscillator panels, two filters and the modulation section fill it. Nine oscillators need a different layout, not a longer one.
- **Torqwave Synth stays what it is:** a full synth, complete as it stands. The new one is a different instrument with a different job, and the guide must describe the two that way, neither as the lesser.

## The question that decides the design

Nine oscillators are easy to add and hard to justify. A single voice with nine oscillators stacked on one filter is mostly a louder, thicker three. What makes nine worth a second instrument is what they are *for*. Three answers, which can be combined:

### A. Three layers of three

- Nine oscillators as **three layers, A, B and C**, each a small synth of its own: three oscillators, a filter, an amp envelope, a level and a pan. The layers then meet in a shared filter, the modulation and the effects.
- A layer can be given **a range of keys and of velocities**, so one patch is a bass under a lead, or a soft pad that a hard hit turns into a stab.
- **A patch of Torqwave Synth opens as one layer.** That makes the 3,743 patches building blocks: choose three and you have a sound nobody shipped. This is the cheapest way to make the new instrument rich on the day it arrives.

### B. Sounds that change over time: the "Evolved" of the name

- **Scenes:** up to four snapshots of every knob in a patch, and one control, **Evolve**, that moves between them. Where two scenes differ, the sound glides from one to the other; a macro, an envelope, an LFO or the mod wheel can drive it.
- **An evolve lane:** a line drawn over bars of the session's tempo that moves Evolve by itself, so a pad opens over sixteen bars or a bass mutates through a phrase, the same each time it is rendered.
- This is the piece that gives the instrument its own character for dark, driving electronic music: long builds, slow corrosion, a sound that is not the same at bar 16 as at bar 1. It needs no new kind of oscillator.

### C. Oscillators that work on each other

- **A nine-by-nine grid:** any oscillator can modulate any other's phase (FM), ring it, or sync it, by an amount in the grid. Metal, bells, growls and noise that three oscillators with one FM warp cannot reach.
- The most new sound, and the most ways to make an ugly one. It costs the most care in the factory patches, and the grid must be drawn so a patch can be read.

**Recommended: A and B in the first version, C as the second.** A makes it useful at once from what exists. B is the reason for the name. C is a deeper instrument and deserves its own stretch of work once the first is in use.

## What it would have

- **Nine wavetable oscillators** in three layers, each oscillator as Torqwave Synth's is (Position, warp, unison, level envelope, a table from a dropped sound).
- **A filter for each layer and one for the whole,** from the filter types there are.
- **Six envelopes, four LFOs, thirty-two modulation slots, eight macros.** More sources than Torqwave Synth because there is more to move.
- **Four scenes, the Evolve control and its lane.**
- **The effects there are,** plus a place for each layer to be sent to them by its own amount.
- **Sixteen voices to start with.** At the worst case measured above, nine oscillators in 8-voice unison on sixteen voices come to about a fifth of one core by simple scaling; that is an estimate to be replaced by a measurement in Phase 0. A patch says how many voices it wants, and unison is counted against a limit so no patch can ask for more than the machine can give.
- **Shared with Torqwave Synth:** the oscillator, filter, envelope and effect code, the wavetables, the playing keys, Audition, favourites, the presets bar and the rules above. The engine is reused, not copied, so a fix to one reaches the other.

## The window

- Nine oscillators cannot each have a full panel. The proposal: **a bank of nine small scopes in a three-by-three grid,** a row a layer, each showing its wave moving and its level; click one and its controls fill a detail panel beside the grid. Layers have a strip each for filter, envelope, range, level and pan.
- **Scenes and the evolve lane along the bottom,** with the playhead moving through the lane as the session plays.
- The look is the signal console's, as Decrypt's page is: scopes and lines, not skeuomorphic knobs alone.
- **A mock-up comes first,** before any window code, as for every new look.

## Patches

- **None of the 3,743 carries over as it is,** and all of them carry over as layers.
- **Factory patches of its own:** about 300 to start, hand-made, each using what is new: layers split or stacked, scenes that differ, an evolve lane. Every category opens with ones that move. Each is measured for level, register and clicks as Torqwave Synth's are, and a first batch is listened to before the rest are made.
- **Built from the others:** a command that makes a patch from three Torqwave Synth patches, one a layer.
- This is the largest single cost. A synth with nine oscillators and thirty thin patches would be worse than not shipping it.

## What it would not do, in a first version

- **Decrypt.** Its search is built on three oscillators; nine is three times the space. A later piece could match with one layer and leave the others to the user.
- **Per-layer outputs.** One stereo output, as every built-in instrument has.
- **More than a small set of automatable parameters.** The macros, Evolve, the layers' levels and the main filter: about twenty. The rest is kept in the patch and reached by modulation and the macros, so the automation menus are not flooded as Basic Drums' are.

## Phases

1. **Phase 0, about two days: a mock-up and a measurement.** The window as a picture for the user to judge. The engine with nine oscillators and three layers sounding, with no window, to measure what a voice really costs and to hear whether three layered patches make something worth having.
2. **Phase 1, about six days: the engine.** Layers with their ranges, filters and envelopes; the larger modulation section; scenes and Evolve; state, presets and the tests the other synth has (no step in the sound anywhere, the same render at any block size, notes that end).
3. **Phase 2, about six days: the window,** from the approved mock-up, with the evolve lane.
4. **Phase 3, about six days: the patches,** the layering command, audition patterns, the guide's chapter and the site's clips.
5. **Later: the nine-by-nine grid (C), and Decrypt for a layer.**

About four weeks to a first release, most of it the window and the patches.

## Risks

- **Two synths that overlap.** If the new one is only "more", it makes the first look unfinished and splits the patches. Scenes and layers are what keep it a different instrument; without them it should not be built.
- **Patch quality at volume.** Hundreds of nine-oscillator patches made quickly will sound alike. Fewer and better, listened to.
- **CPU on older machines,** Intel Macs most of all. The voice limit and the count of unison must hold.
- **The name.** "Evolved" beside "Torqwave Synth" can read as its replacement. The guide, the site and the instrument menu must present two instruments; a name of its own (as StringSwarm has) would avoid the question.
- **Test time.** The full suite grows with every instrument; its measured checks must be chosen, not copied nine times over.

## Decisions for the user

1. A and B first, with the oscillator grid (C) after: is that the instrument you meant, or is the grid the point?
2. Three layers of three, or nine free oscillators with no layers?
3. The name: Torqwave Synth Evolved, or a name of its own?
4. About 300 hand-made patches to start, plus everything built from the existing ones as layers: enough?
5. Go for Phase 0, which ends with a mock-up and real numbers before anything is committed to?
