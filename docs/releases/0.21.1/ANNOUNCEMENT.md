# Announcing Torqwave 0.21.1

A draft, not posted. 0.20.0 was the last release set against in public, and 0.20.1 and 0.21.0 were not announced, so this announces what is new since 0.20.0, with 0.21.1's drum sampler first. It is one long post of text, then the pictures in posts of four under it, for people to click through. An Instagram caption for the infographic in this folder follows.

The post is about 4,300 characters, so it needs an account that can post long posts. The pictures are in [compare/graphics/](../../compare/graphics/) (made from the [comparison data](../../compare/README.md)), [screenshots/](../../screenshots/) and this folder.

## The post

```text
Torqwave 0.21.1 is out.

Torqwave is a free, open-source audio editor and DAW in one program, for macOS, Windows and Linux. You edit a file down to its spectrum, repair and master it, then build a session around it with instruments, a piano roll, automation and video, without leaving the program.

Here is what has landed since 0.20.0, newest first.

THE DRUM MACHINE IS A DRUM SAMPLER TOO

Basic Drums started as 30 synthesized drums on pads. In 0.21.1:

• Any pad plays your own samples. Drop sounds on a pad: up to 16 layers by velocity, with round robin, so soft and hard hits are different recordings and a repeated hit isn't the same one twice.

• 64 pads. The 30 sounds, and 34 pads of your own, each with its name, colour and note.

• Five effects on every pad: an envelope, a transient shaper, a saturator with four curves, a four-band EQ and a compressor, beside the filter, crusher and sends each pad already had. Off until you switch them on.

• 24 more synthesized sources, 54 in all: sub, rumble and FM kicks, metal and noise snares, anvils, pipes, zaps, machine hits. Any pad can play one. Fifteen kits.

• Choke groups of your own, and a mixed kit is fine: a sampled kick, a synthesized snare, and a sampled open hat cut by a synthesized closed one.

• Bring in a kit you already have. It reads Battery 4 kit files you own, with the samples left where they are, or lays a folder of sounds on the pads by their names. A report says what it approximated and what it left out.

No samples ship with it. The factory kits are all synthesized.

ALONGSIDE THE OTHER INSTRUMENTS

Basic Drums sits beside Torqwave Synth (a wavetable synth with 3,743 patches, and Decrypt, which builds a patch from a recording of one note), Torqwave Sampler and StringSwarm, with 1,032 sequence presets to play on them. Since 0.20.0 the drums also have a second, Modern model for 21 of the 30 sounds, and the synth has a Panic for a note that won't stop.

IT STARTS WHERE YOU STOPPED

New since 0.20.0: Continue where you left off. Quit with files open in the editor and a session in the multitrack view, and the next start brings all of it back, each where you were, including a session you never saved.

ASK TORQ

Describe an edit in a sentence and a language model you run yourself plans it; nothing changes until you read the plan and press Apply. It works with any server that speaks the OpenAI API, on your own machine. New since 0.20.0: it remembers the conversation, so "now a little less" works, and it shows how much of the model's context you've used.

WE MEASURED OUR OWN EFFECTS, AND FIXED WHAT FELL SHORT

For the website I rendered before-and-after clips of Torqwave's repairs and published the numbers, including the ones that weren't good enough. Three got fixed:

• The Maximizer let true peaks through 0.2 dB over its ceiling. It holds the ceiling now, on the same meter you check it with.
• Click and Pop Repair missed 16 of 60 test clicks, and was rewriting clean drum hits it mistook for clicks. It finds all 60 now and leaves drums alone.
• Repair Clipping drew a smooth curve over each clipped peak. It now rebuilds from the sound around the clip. It's better; on drums it is still mostly a guess, and the page says so.

HEAR IT BEFORE YOU DOWNLOAD IT

The website has over a thousand clips rendered by Torqwave's own code with nothing added: synth patches, every drum kit and every new drum source, sequences, and the repairs with their measurements.
https://bobclawblaw.github.io/torqwave/

WHAT IT DOESN'T DO

No audio to MIDI, no CLAP or ARA, no Dolby Atmos. The drum pads share one stereo output, and their effects can't be automated. The kit import brings over cells, samples, layers, tuning, envelopes and chokes, not another program's effects or modulation. The installers aren't notarized yet, so macOS asks before it opens the first time. The Windows and Linux builds are made by automation and haven't had the hands-on time the Mac build has. I keep a feature comparison with 15 other editors and DAWs, read from their own documentation, and it lists what Torqwave lacks as plainly as what it has.

Free. Open source, AGPL-3.0. No account, no subscription.

Download: https://github.com/BobClawblaw/torqwave/releases/tag/v0.21.1
```

## The pictures, in order

Posted under the text, four to a post.

**First four: the release, and the drums**
1. `docs/compare/graphics/1-release-by-the-numbers.png`
2. `docs/screenshots/guide-drums.png`
3. `docs/screenshots/guide-drums-sample.png`
4. `docs/screenshots/guide-drums-effects.png`

**Second four: more drums, starting, and measuring**
5. `docs/screenshots/guide-drums-sources.png`
6. `docs/screenshots/guide-drums-import.png`
7. `docs/screenshots/guide-continue.png`
8. `docs/screenshots/site-quality.png`

**Last four: where it stands**
9. `docs/compare/graphics/5-where-torqwave-stands.png`
10. `docs/compare/graphics/2-against-the-editors.png`
11. `docs/compare/graphics/3-against-the-daws-1.png`
12. `docs/compare/graphics/4-against-the-daws-2.png`

A reply under them can carry the comparison's address: `https://github.com/BobClawblaw/torqwave/blob/main/docs/compare/MATRIX.md`.

## Alt text

The pictures that are new or changed in 0.21.1 are below. For Continue and the Quality page, use the alt text in [ANNOUNCEMENT-0.21.0.md](../../announcements/ANNOUNCEMENT-0.21.0.md). Graphics 2 to 5 set 0.21.1 against 0.20.0; use their alt text from the same file, reading 0.21.1 and 0.20.0 for the two Torqwave versions (no row differs between the two).

For graphic 1, `1-release-by-the-numbers.png`:

```alt
A dark chart titled "Torqwave 0.21.1: what it adds over 0.20.0". Six tiles, each with a number and the figure for 0.20.0 under it: 64 drum pads (30 before), 54 drum sources (30 before), 15 drum kits (7 before), 16 layers a pad (0 before), 8 choke groups of your own (0 before), 1,032 sequence presets (1,018 before). A list beside them, "Also new since 0.20.0": sample pads, your own sounds on any drum pad, in layers; on each pad an envelope, transient shaper, saturator, EQ and compressor; imports a folder of sounds, or a Battery 4 kit you own; Modern types for 21 drum sounds; Continue where you left off, unsaved sessions too; Ask Torq remembers the conversation; Maximizer, Click Repair and Repair Clipping corrected; Panic, every instrument silenced.
```

For the infographic, `infographic-portrait.png` (and `infographic-square.png`):

```alt
A dark poster headed Torqwave 0.21.1, "Basic Drums: a drum synth and a sampler in one". A grid of 64 pads, the first 30 coloured, beside a large 64: 30 drum sounds, 34 for your samples. Five tiles: 54 synthesized sources; 16 velocity layers a pad, round robin; 5 effects on every pad; 15 kits; 8 choke groups of your own. Under them: the five are envelope, transient shaper, saturator, EQ, compressor; imports a folder of sounds, or Battery 4 kit files you own; also an unsaved session comes back on Continue, and a Panic command. At the foot, bobclawblaw.github.io/torqwave and "Free, open source".
```

For `docs/screenshots/guide-drums.png`:

```alt
The window of Basic Drums in its Bunker kit: the kit selector and presets along the top with the bus and space beside them; 64 pads, eight across, the first 30 coloured by family and named for what they play (Rumble Kick, Noise Snare, Clap Stack, Noise Hat, FM Tom, Dark Ride and the rest), four of the pads after them playing Sub Drop, Zap, Noise Burst and Tin, the others empty; and the selected Rumble Kick's panel on the right with its hit drawn, its Tune, Decay, Tone, Level, Pan and Drive knobs, and the six effect tabs Filter, Env, Transient, Saturate, EQ and Comp.
```

For `docs/screenshots/guide-drums-sample.png`, `guide-drums-effects.png` and `guide-drums-sources.png`:

```alt
Basic Drums with a pad that plays samples selected: its waveform, a list of its layers each with a name and a range of velocities, a line saying the velocity of the last hit and which layer it played with that layer's row lit, and the Add, remove, up, down and Spread buttons over the pad's knobs.
```

```alt
Basic Drums with a pad's Effects on the EQ page: six tabs, Filter, Env, Transient, Saturate, EQ and Comp, a dot lit on the ones that are on; the EQ's curve with a numbered point for each of its four bands; and the band's Freq, Gain and Width knobs.
```

```alt
Basic Drums with the selector at the top of a pad's panel open: the pad's own types, Sample, and the 24 synthesized sources listed by family: kicks, snares and claps, hats and cymbals, metal, industrial and low.
```

For `docs/screenshots/guide-drums-import.png`:

```alt
Basic Drums after a folder of sounds was imported, with a report over the window: "Imported Factory Floor. 13 pads from 16 sound files", how many were placed by an instrument's name, by a note's name and on notes of their own, that two pads have several layers from files alike but for a mark, and a line saying to check the pads by ear because names are a guess at what a file is. The folder and its sounds were made up for the picture.
```

## Instagram

The infographic is in this folder (portrait, 1080 by 1350). A caption for it:

```text
Torqwave 0.21.1: the drum machine is a drum sampler too.

64 pads. Your own samples in up to 16 velocity layers, with round robin. An envelope, transient shaper, saturator, EQ and compressor on every pad. 54 synthesized drum sources and 15 kits, no samples shipped. Bring in a Battery 4 kit you own, or a folder of sounds.

Torqwave is a free, open-source audio editor and DAW in one program, for macOS, Windows and Linux. No account, no subscription.

Hear it before you download it: link in bio (bobclawblaw.github.io/torqwave)

#torqwave #musicproduction #drummachine #sampler #sounddesign #techno #industrialmusic #ebm #opensource #daw
```

## Notes for whoever posts it

- Post only once the release is published and its Mac, Windows and Linux packages are on the release page, so "is out" and "for macOS, Windows and Linux" are true. The Windows and Linux packages are built by the workflows and not tried by hand, which the post says.
- The figures: 64 pads (30 sounds and 34 user pads), 16 layers, five effects, 24 new sources and 54 in all, fifteen kits, 3,743 patches, 1,032 sequence presets, 116 starter kits and 37 styles are the release's own counts. "16 of 60" and "all 60" are Click and Pop Repair on a pad and bell clip with 60 clicks put in; in a drum loop it repairs 58 of 60, which the post does not claim. "0.2 dB over" is 0.21 dB on Torqwave's loudness meter.
- The kit import: it read all 5,227 Battery 4 kit files on the development machine's sample drive and kits saved by a current Battery 4. Reverse, the volume envelope and chokes were confirmed against kits saved for the purpose. Colours, effects and modulation do not come over.
- The comparison's total is 45 of 53, unchanged since 0.21.0: the drum sampler deepens a row that was already a yes. The pictures set 0.21.1 against 0.20.0.
- The post says "I": it is written to be posted by the project's author.
- It calls no other product worse, and says what Torqwave lacks.
