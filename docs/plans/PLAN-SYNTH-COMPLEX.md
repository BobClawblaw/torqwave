# Plan: 500 Complex patches for Torqwave Synth

A scope, not yet started. Asked for on 2026-11-25, with nine oscillators being built into Torqwave Synth: 500 new sounds in a new folder, **Complex**, each using four or more oscillators, with at least five in every category using all nine, made to stand up in a club for techno and electronic industrial music.

It depends on the nine-oscillator engine and window, which come first. The 3,743 patches there are stay exactly as they are.

## What "Complex" is

- **A folder in the patch list,** after Favourites and Decrypted and before the categories, with the synth's own categories inside. The arrows step through a category of it as they do anywhere else.
- **Every patch uses at least four oscillators.** At least five in each of its categories use all nine.
- **500 patches across all 29 categories** of the synth (every one but Init), as the user asked: 18 in each of the seven a club track leans on most (Bass, Reese, Acid, Lead, Stab, Sequence, Pad) and 17 in each of the other 22 (Sub, Chord, Pluck, Arp, Keys, Organ, Guitar, Brass, Strings, Drone, Bell, Riser, Impact, Cinematic, Perc, Drum, FX, Glitch, Game, Chip, Texture, Vox).
- **At least 145 of the 500 use all nine oscillators** (five in each of 29 categories). The proposal is seven a category, 203, because the nine-oscillator patches are the ones that show what the synth now does.
- **The categories that are not club sounds are made for the same floor.** A Complex organ is a rave organ stab, strings are a detuned wall, brass is a hoover's cousin, keys are a broken electric piano through drive, and Game and Chip are sounds for a game's score and its machines with the weight of the rest: nothing polite.

## Sounds that have not been heard before

The user's words: "I want to hear sounds that have never been heard before. Even for games." Nobody can prove that of a sound. What can be done is to make them unlike what Torqwave Synth has, on purpose, and to measure that:

- **Things three oscillators could not do.** Oscillators that are never heard and only drive others' FM, two and three deep, which the new FM From choice allows. Seven layers that take turns by their own level envelopes, so one held note is a phrase. A strike, a body and a tail that are three different instruments.
- **Tables made from sounds, not waves.** A table taken from one of Torqwave's own drum hits, a StringSwarm chord, a slice of noise or another patch's render, then read as an oscillator. Each such table is rendered by Torqwave itself, so nothing is borrowed.
- **Roles put where they do not belong:** a bell's strike on a bass, a vowel under a kick, a riser's noise as a lead's width.
- **Measured, not claimed:** every Complex patch must be further than a set distance from every one of the 3,743 patches there are, by the measure Decrypt uses to compare two sounds. A patch that lands near an existing one is changed or dropped. That makes "not heard before in this synth" a test that fails, and leaves "never heard anywhere" as the aim it is.

## What makes nine oscillators sound like more than a thick three

Nine oscillators on one note turn to mud unless each has a job. Every Complex patch is built from **roles**, and its description names them:

- **Weight:** a sub or a low sine, mono, in the middle, under everything.
- **Body:** the tone you would call the sound, one or two oscillators.
- **Bite:** a harder, brighter layer that speaks at the start of a note: a sync sweep, an FM strike, a fold.
- **Width:** a detuned unison pair out at the sides, kept out of the low end.
- **Air:** something above 6 kHz that opens with the filter or a macro.
- **Grit:** noise, a crushed or folded layer, a metal table, low in the mix.
- **Movement:** layers that come and go by their own level envelopes, LFOs or step patterns, so a held note changes.
- **Strike:** a short layer that is only the first 30 ms.
- **Modulators:** oscillators that are not heard, only driving another's FM, which the new FM From choice allows.

A four-oscillator patch takes four of these. A nine-oscillator patch takes them all, or doubles some an octave apart.

## What "club ready" is held to

Measured for every patch by the tests, not left to taste:

- **The low end is mono.** Below 120 Hz the two channels match, so it survives a club's summed subs.
- **Level:** every patch within a set range of the others at the same velocity, so stepping through them is not a jump in loudness, and none clips at full velocity with all its voices.
- **Weight where it belongs:** bass, sub, reese and acid patches have their energy below 200 Hz at the notes they are meant for; leads and stabs sit above it and leave it clear.
- **It cuts:** hooks have energy from 1 to 5 kHz at their start, so they are heard over a kick and hats.
- **No clicks, no DC, no step** at a note's start, end, restrike or release, as every patch is held to now.
- **Four macros that change the character,** each named, each moving the sound by an amount the test can measure.
- **Eight notes at once, at least.** The synth sounds at most 384 unison voices at a time, which is what three oscillators could ask for and keeps its worst cost where it is today. A patch asking for more plays fewer notes. So a Complex patch uses at most 48 unison voices a note, counting every oscillator that is on or driving another: nine oscillators have about five each to share, not eight. A chord stab that dropped notes would be heard.
- **Different from each other:** no two patches in a category nearer than a set distance by the measure Decrypt uses to compare sounds, so 25 basses are 25 basses and not one bass 25 times.

And heard, because measurements do not make a sound good:

- **Each patch gets a clip** in a club setting: its own phrase over a plain four-to-the-floor kick and hats at 132 BPM, and alone.
- **The first 58 are made and listened to before the other 442,** two from each category, with the user's verdict on what is right and wrong about them. What is learned sets the recipes for the rest.
- **Then in batches of about 100,** each with its clips, and a short list of the ones the maker thinks are weakest.

## How they are made

- **By hand, from recipes.** A recipe is one idea for a category (a rolling sub-and-click bass, a hoover stab, a metal sequence, a pad that corrodes); about 90 recipes, three or so a category, each giving 5 or 6 patches that differ in more than a knob: other tables, other roles doubled, other movement.
- **Not by a generator turning knobs at random.** 2,463 of the 3,743 patches there are came from banks made by rule; Complex is to be the other kind throughout.
- **Names** in the manner of the ones there are (Turbine Throat, Hydraulic Maw): two words, no numbers, none repeated.

## What else it touches

- **Decrypt** plays factory patches as starting points for a three-oscillator search. Complex patches are left out of that search until Decrypt can work with more than three.
- **Audition, favourites, the presets bar and Ask Torq's search of sounds** take the new patches as they take any; Ask Torq's index is rebuilt.
- **The site:** clips of a selection on the Sounds pages, not all 500.
- **The guide and the README:** the counts (4,243 patches), the folder, and what a role is.
- **The download** grows very little: a patch is a few hundred bytes of settings.

## Tests

In `--synth`, for every Complex patch: four or more oscillators on, and nine where it says nine; the count of nine-oscillator patches in each category at least five; each of the measures above; a name that is unique; the patch saved and loaded back the same; and the whole folder rendered at two block sizes to the same result. The clips are made by a command of their own.

## Time

About fifteen working days after the engine and window are in, by the estimate of the session that made the synth's layered patches, which took several rounds of fixes after their first pass (near-copies, loud outliers): one for the folder in the browser, the tests and the clip command; one and a half for tables in factory patches; one and a half for the first 58 and their clips; then, after the user has heard them, ten to eleven for the other 442 in batches. Twelve is the least it could take. The listening is the user's time, about an hour for the first 58 and less for each batch after.

## What the engine needs first

Found by asking the session building the nine oscillators what this plan asks that the engine cannot yet do:

- **An oscillator that is only a modulator.** Today an oscillator at no level is not computed, so it could drive nothing. An oscillator named by another's FM From will be computed and kept out of the mix. Being done with the engine.
- **Tables in factory patches.** A factory patch holds settings and macro names only; tables from sounds exist for the user's presets and Decrypt. Each Complex patch that needs one will carry its table in the patch itself (a few kilobytes), so it is the same on every machine and loads at once.
- **A folder level in the patch list,** which touches the arrows, favourites and Audition's choice of pattern.
- **The distance from every existing patch** needs each of the 3,743 measured once and kept, not rendered again for every new patch.

## Risks

- **Novelty against polish.** A sound nobody has heard is often one nobody wanted. Each patch still has to pass the club measures and be playable from the keys; the strange ones are kept strange in character, not in level or tuning.
- **Sameness.** 500 is a lot. The distance test catches near-copies; only listening catches dull ones.
- **Mud.** Nine oscillators phase against each other. The roles, the mono low end and the level checks are there to prevent it, and some nine-oscillator patches will have to be thinned to sound bigger.
- **Cost.** If the engine's measured cost for nine oscillators is higher than the estimate, the heaviest patches get fewer voices or less unison, and the count that use all nine stays at the minimum.
- **Taste.** "Club ready" is the user's ear in the end. The first 58 are the cheap place to find out what that means.

## Decisions for the user

1. Seven nine-oscillator patches a category (203) where five was asked for: yes?
2. The first 58, two a category, heard before the rest are made?
3. Go, once the nine oscillators and their window are in?
