# Plan: samples in the drum machine

A scope, not yet started. Asked for on 2026-11-24: a drum sampler joined to Basic Drums, of the kind a dedicated drum sampler plug-in is, and, if it can be done, a way to load Battery kits.

## Where things stand

- **Basic Drums** is a synth. It has 30 sounds on fixed pads, on the General MIDI drum notes 35 to 81. Each sound has a Type (Classic or Modern), its own tune, decay, tone, level, pan and drive, a filter, a crusher, and sends to the kit's reverb and delay. A kit is the settings of all 30 sounds and the bus. Kits, sequences, the drum grid, drum maps and Remap Notes all rest on those 30 sounds and their notes.
- **Torqwave Sampler** already does the playing: samples whole in memory from any file Torqwave opens, shared between instruments, zones by key and velocity (up to 128), one-shots, choke groups, slices, and a way to find samples that have moved. It has no kit of named pads and no per-pad channel strip.
- So the engine work is mostly joining two things that exist. The new work is the pad's layers, how a kit keeps its samples, and the window.

## What is proposed

### Phase 1: a pad can play samples (about four days)

- **A third Type on every pad: Sample**, beside Classic and Modern. All 30 pads get it, including the nine that have no Modern type.
- **Drop a file on a pad** (from Finder, the Files panel, the Sound Library, or an editor selection) and the pad plays it. Drop several and they become layers.
- **Layers on a pad, up to 16:** each with a velocity range, so soft and hard hits are different recordings, and **round robin** among layers that share a range, so a repeated hit is not the same recording twice. Round robin is new; the Sampler does not have it.
- **The pad's own controls keep working on a sample:** tune (repitch), decay (a fade to nothing from the hit, with the sample's own length the longest), tone (a tilt), level, pan, drive, the filter, the crusher and the two sends. Plus three for samples only: start, reverse, and a pitch envelope amount.
- **Chokes** stay as they are (hats, triangle, cuica) and work across types, so a sampled open hat is cut by a synthesized closed one.
- **A kit keeps its samples by reference**, the way the Sampler does, with the same search for ones that have moved. **Save Kit with Samples** writes the kit and copies of its samples into one folder, for moving a kit to another machine.
- **Mixed kits:** a kit can have a sampled kick, a Modern snare and Classic hats. Everything that plays a kit today (sequences, the grid, drum maps, Remap Notes, Ask Torq) needs no change, because the pads and notes are the same.
- **Sessions and presets saved before** load as they do now: no pad has samples until one is given some.

### Phase 2: more pads than thirty (about four days)

- **User pads:** up to 34 more, for 64 in all, on notes outside 35 to 81 or sharing a note with a built-in pad (layered kicks). Each is a Sample pad with its own name and colour.
- **Choke groups you set,** eight of them, beside the three built in.
- **The window:** the pad grid scrolls or pages to the user pads; the selected pad's panel shows the layers as bars across the velocity range, each with its waveform, and Classic and Modern keep the picture they have now.
- **Per-pad outputs** are left out: Basic Drums has one stereo output, as every built-in instrument does. That would be its own piece of work.

### Phase 3: Battery kits (looked into: readable; about six days to build)

- **What is known.** Battery 4 saves a kit as an `.nbkt` file and a cell as `.nbcl`. The format is binary and not documented, and no open reader for it turned up in a search on 2026-11-24. An older, unrelated script exists for Battery 1's `.kit` files only. No Battery kit could be read on the development Mac to look inside one.
- **So the first step is to look, not to build:** take a few kits of the user's own, saved from Battery with known settings, and see whether the cells, their sample paths, velocity ranges, tuning, levels and chokes can be read reliably. Two days gives a yes, a partial or a no.
- **What an import would and would not bring over, if yes:** the cells and their samples, layers and velocity ranges, tune, level, pan, choke groups and the notes they are on. Not Battery's effects, its modulation, its time-stretching or its articulations: those have no counterpart here, and the import would list what it left out.
- **Samples.** An import reads samples from where the kit points. A kit's samples are the property of whoever licensed them; Torqwave would read them in place and never copy or ship them, and Save Kit with Samples would be for the user's own use. A kit whose samples are inside a protected container could not be read at all.
- **A fallback that needs no format work:** drop a folder of samples on the kit and Torqwave lays them on pads by name (kick, snare, hat and so on, and GM note numbers or note names in the file names). It covers most kits bought as folders of WAV files, and a Battery library's samples where they are plain files.
- **Other kit formats** worth more than Battery's for the same effort, because they are documented: SFZ drum kits and Ableton-style folders of one-shots. Worth deciding before Phase 3 starts.

#### What the two days found (2026-11-25)

The answer is **yes** for what a drum pad needs, and partial for the rest. The first two points under Phase 3 above were written before any kit could be opened; this section replaces them.

- **The file.** A kit is a nested container of tagged chunks with a size in front of each. One chunk holds nearly everything, packed with a byte-oriented LZ77 scheme (the FastLZ level 2 stream layout, which is public and MIT-licensed; the reader here would be our own). Unpacked, it is a tree of numbered records: a program, 128 cells, a list of zones (one per sample layer, each naming its cell), a list of voice groups and a table of sample paths. A cell file (`.nbcl`) is the same container with the block stored unpacked.
- **How it was checked.** A reader script of about 170 lines was run over every kit found on the user's sample drive: the 143 factory kits and 5,084 kits from 126 other libraries, written by six versions of the program record and two of the zone record. All 5,227 parsed, and all 117,066 sample references resolved to a file on disk. The samples are plain WAV files (16- and 24-bit and 32-bit float, 44.1 to 96 kHz); three references in all were to REX loops, which would be skipped. No kit saved by the user was found, so user-saved kits are untested.
- **Sample paths** are stored relative to the kit file (up some folders, then down by name), not absolute and not by library id, so a library that has been moved whole still resolves.

| Setting | Read | Notes |
|---|---|---|
| Cell name | Yes | Text, in the cell record |
| Sample file per layer | Yes | Index into the path table; every one resolved |
| Velocity range per layer | Yes | Low and high, 1 to 127, with crossfade widths |
| Note that triggers the cell | Yes | Low and high key; the cell's and its zones' agreed in all 7,172 factory cells compared |
| Level, pan | Yes | Per cell and per layer, as gain and -1 to 1 |
| Tune | Yes | Per cell and per layer, as a frequency ratio; root key is there too |
| Start and end | Yes | Per layer, in frames; the end as a distance back from the sample's end |
| Choke | Yes, with one rule taken on trust | Cell names a voice group; a voice group has a voice limit and an exclusive group. Cells sharing a one-voice group, or groups sharing an exclusive group, cut each other. The hi-hats of the kits looked at fit this |
| Reverse | Partly | One of four flags beside the tune; which one is inferred, not proven |
| Colour | Partly | An index into Battery's palette (kicks, snares, hats and so on each have their own); the colours themselves are not in the file, so a small table would be made by eye |
| Volume envelope | Partly | Six numbers per cell and an on/off flag are there; which is hold, decay and release is inferred from their ranges |
| Round robin | Not there | No kit looked at uses one. Layers that overlap are velocity crossfades (163 factory cells) or have no crossfade (150: stacks on one range, or ranges that share an edge value) |
| Effects, modulation, stretch, articulations, loop slices, buses | Readable as blocks, not imported | No counterpart here |

- **What would be approximated.** A velocity crossfade becomes a hard split at the midpoint. Stacked layers (several samples on one velocity range) become round robin or the first layer only, the user's choice at import. The envelope becomes the pad's decay. A cell spread over several keys (about one in ten cells in the other libraries, three in the factory library) lands on its lowest note with its tune kept. Per-layer level and tune are folded into the layer where a pad has them, or dropped with a note in the import's report.
- **What would not fit.** 215 of the 5,227 kits have more than 64 cells with samples, and four have a cell with more than 16 layers (one has 127). The import would take the first 64 cells in order and the 16 layers spread most evenly across the velocity range, and say so.
- **Before the build: half a day of probe kits.** Ten small kits saved from Battery with one setting changed in each (reverse, each envelope stage, a stack, a choke, each palette colour) turn every "partly" and the trusted choke rule into a checked fact. They are compared and thrown away, never committed.
- **Estimate, with tests: about six days.** Half a day of probes; two days for the reader in C++ (container, unpacking, records, paths, with bounds checks on every length since the file is untrusted input); two days to map cells onto pads and layers, with the import command, the report of what was left out and a prompt to find a moved library; a day and a half of tests. The tests build a small synthetic kit in memory with the reader's own writer, so no licensed file is ever in the repository.
- **Risks.** The format is not documented and a future Battery could change it; the reader checks the record versions it knows (six program versions, two zone versions so far) and refuses others plainly. Kits from Battery 3 and earlier use different file types and were not looked at. Kits whose samples are in a protected or compressed container (as some sampler libraries ship) cannot be read; none of the 5,227 was like that. The folder-of-samples import stays worth having as the fallback for those.

## What it would be called

It stays **Basic Drums** through Phase 1. With samples, layers and 64 pads the name undersells it; a rename (the engine and its saved states keep their identifiers, as Basic Synth's did) is a decision for the user at Phase 2.

## Risks and costs

- **Memory.** Samples are held whole. The Sampler's limits apply: 10 minutes a sample, 1 GB an instrument. A kit of 64 pads with 16 layers each is far inside that for drum hits.
- **The no-click rule.** A sample cut off by a choke or a restrike fades over a few milliseconds, as synthesized hits do; a sample that starts off zero gets the same rise the Sampler gives. Both are tested the way the synth's are.
- **Block-size independence.** Basic Drums renders the same at any block size and seeds its noise per note. Round robin must be counted per note-on, not per block, to keep that.
- **Factory content.** No factory samples are proposed: the factory kits stay synthesized, so the download does not grow and nothing licensed is shipped.

## Tests

Each phase adds to `--drums`: a sampled pad's level, start and end without a step; velocity layers and round robin choosing as they should; chokes across types; a kit with samples saved and loaded, with a moved sample found; the same render at any block size; an old state loading unchanged; and for Phase 2 the user pads' notes through the grid and Remap Notes.

## Decisions for the user

1. Phase 1 alone first, tried in the program before Phase 2 is started?
2. Sixteen layers a pad and 64 pads: enough?
3. Battery import: the two days are spent and the kits are readable (see Phase 3). Build it (about six days), or take the folder-of-samples route and SFZ first?
4. The name, at Phase 2.
