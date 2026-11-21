# Crossfade plan: editing crossfades in a session

Torqwave's sessions crossfade overlapping clips, but only as two separate fades that happen to have the same length. Cubase and Nuendo treat a crossfade as one thing: it can be dragged as a piece on the lane, opened in an editor that shows both curves, auditioned, and made from two clips that only touch. This plan adds that, built our own way.

Each piece below has:

- what it is;
- what it builds on in the code today;
- its size: **S** is days, **M** about a week, **L** more;
- how it is tested.

Each step lands as its own commit with its tests, guide pages and a changelog entry that says what was not tried by hand. The steps are built in order, and each one is useful on its own. Problems found by using them go into the [fix log](DOGFOODING.md) with an issue, as each is fixed.

## What the code gives us to build on

- **Clips and fades** (`Source/Session.h`, `SessionClip`): `position`, `offset` (the first frame of the audio it plays), `length`, `fadeIn` and `fadeOut` in frames, and `fadeInShape` and `fadeOutShape`. The shapes are in `Source/FadeShape.h`: S-curve (raised cosine), linear, equal power (a quarter sine), slow and fast.
- **Crossfade Overlaps** (`X`, `SessionEdits::crossfade` in `Source/Session.cpp`): after a clip is dragged or trimmed partly over another on its track, it sets the outgoing clip's fade out and the incoming clip's fade in to the overlap's length. The setting "Crossfade at edits" is the same switch.
- **The mixer** (`SessionMixer.cpp`) multiplies each clip by its fade-in and fade-out levels. Where clips overlap with Layer Overlapping Clips on, the one on top still lets the one under it through over its own fades, so a crossfade sounds both.
- **The lane** (`MultitrackView.cpp`) draws each fade's curve (`fadeCurve`), and a clip's fade handles drag its fades. A right-click on a fade picks its shape. Double-clicking a clip opens it in place in the editor.
- **Comp joins** have their own equal-power crossfade (`compFade`). They are not part of this plan.
- **Effect dialogs** (`showEffectDialog`, `EffectPreview`) already preview a change before it is applied. The Crossfade Loop dialog in the editor is one of them.

## The unit: a crossfade

A crossfade is two clips on one track where the earlier clip's end overlaps the later clip's start, the earlier clip's fade out covers exactly the overlap, and the later clip's fade in does too. It is not stored: it is found from the clips. So sessions are written as before, and older versions of Torqwave open them with the same sound.

Its **join** is the middle of the overlap. Its **length** is the overlap's length. A **handle** is the audio that a clip's file has beyond what the clip plays: before `offset`, and after `offset + length`. A crossfade can only grow as far as both clips' handles reach.

## Step 1: crossfades in the session's edits (S) — done

- `SessionEdits::crossfadeAt (state, track, frame)` finds the crossfade at a frame, and `crossfadesIn (state, range)` finds every crossfade in a range.
- **Set the length**, keeping the join where it is: the earlier clip ends half the change later and the later clip starts half the change earlier (its `offset` moving with it), each limited by its handle. Both fades follow.
- **Move the join**: both clips' edges move together, and the length stays the same.
- **Set the shapes**: each side's shape, or a matched pair. **Equal gain** pairs whose levels sum to 1 (S-curve with S-curve, linear with linear). **Equal power** pairs whose squares sum to 1 (equal power with equal power).
- **Remove a crossfade**: the clips are cut back to a butt join at the old join.
- **Crossfade Selected Clips** (`Shift-X`): two selected clips on one track that touch or overlap get a crossfade of the default length (10 ms until step 4) centred on where they meet, reaching into their handles. Without handles it is as long as they allow, and refused with a reason when they allow none.
- **Exact pairs:** a clip's fade in starts one step above silence, as the editor's Crossfade Loop does, so the two sides of an overlap sum to exactly 1 (or 1 in power). Before, they summed to one step short of 1 throughout, a dip of 1/length.
- Each edit is one undo step, and locked clips are left alone.
- Tested:
  - the edits above against hand-worked frame values, including at the handles' limits, for locked clips, and with the tempo map;
  - an equal-gain crossfade of a clip into a copy of itself plays the original sample for sample (worst difference reported);
  - an equal-power crossfade of two uncorrelated noise clips keeps its RMS within 0.1 dB across the join;
  - a removed crossfade leaves a butt join with no fades;
  - a session written with crossfades reads back the same, and its file has no new fields.

## Step 2: crossfades on the lane (M) — done

- Over a crossfade, the lane shows the overlap as one shaded region with both curves. The pointer changes over its edges and its middle.
- **Drag an edge** to set the length (symmetric about the join), and **drag the middle** to move the join. Both snap like clip edges do, and the status line shows the length in ms (and in beats with bars and beats on).
- **Right-click** a crossfade for each side's shape, Equal Gain, Equal Power and Remove Crossfade. Its presets and Edit Crossfade... join the menu with the editor in step 3.
- **Double-click** it to open the editor (step 3). Over a crossfade this takes the place of Edit in Place, which still opens on the rest of the clip. Until step 3, a double-click there still opens the clip in place.
- Tested: the tests' mouse drags an edge and the middle and gets the frames step 1 gives; a right-click shape lands on both clips; a double-click over a crossfade opens the editor, and elsewhere on the clip opens the clip in place. Not driven by the tests: the look of the region and its pointers.

## Step 3: the crossfade editor (M)

- A window (**Edit Crossfade...**, or a double-click) for one crossfade, or for every crossfade in the selection at once.
- It draws both clips' waveforms across the overlap with both curves over them, and the sum (gain, or power) as a line, so a dip or bump in level shows.
- Its controls: the **length** (ms, frames, or beats with bars and beats on), each side's **shape**, **Equal Gain** and **Equal Power**, and **Symmetric** (both sides take the shape you set on one).
- **Presets:** the factory ones (Short 10 ms equal gain, Dialogue 40 ms equal power, Music 200 ms equal power, Long 2 s S-curve) and your own, saved and recalled by name like the effect presets.
- **Audition:** **Play Out** (the outgoing clip alone, through the crossfade), **Play In** (the incoming one alone), and **Play Crossfade**, each with a **pre-roll** and **post-roll** in ms. It plays through the track's effects and the session's mix, and changes made while it plays are heard at once.
- Changes apply as you make them, as one undo step when the window closes, and Cancel puts the crossfade back.
- Tested: the editor's controls, driven by the tests, set what step 1's edits set; a preset saved and recalled gives the same crossfade; the audition renders exactly what the session plays over that range (worst difference reported); with several crossfades selected, one change reaches them all. Not driven by the tests: the waveform drawing and audition on a real device.

## Step 4: defaults (S)

- **Settings > Multitrack** (beside Crossfade at edits): the default crossfade length and shape pair, used by Crossfade at edits, by Crossfade Selected Clips, and by new crossfades from a drag. They start as 10 ms equal gain, which is what most editors use for edits within one sound.
- "Crossfade at edits" today sets the fades to whatever the overlap is. With a default length set, an overlap shorter than it is still crossfaded over the overlap, and nothing longer is made.
- Tested: each way of making a crossfade uses the defaults, and the settings come back after a restart.

## Step 5: Ask Torq (S)

- `list_clips` shows each crossfade (the two clips, the join and length in seconds, and the shapes).
- A new `set_crossfade` tool creates, changes or removes one: the length, a move of the join, the shapes or a preset. It is refused with a reason where the handles don't allow it.
- Tested under `--ai` like the other session tools, against step 1's edits.

## Not in this plan

- **Asymmetric crossfades** (a fade out longer than the overlap, or a fade in shorter): the fades stay separate settings, so this can follow later without changing the files.
- **Drawn curves:** curves with points of their own, as in Cubase's spline mode. The five shapes come first.
- **Crossfades across tracks** and comp joins, which have their own crossfade.

## Last

- Guide: the session chapter's fades and crossfades section, with a screenshot of the editor and one of a crossfade on the lane, taken in bars and beats with everything on the grid.
- The shortcuts list gets `Shift-X`, and `python3 Scripts/check_commands.py` passes.
- The changelog gets a section, and the README a line if it lists session editing.
