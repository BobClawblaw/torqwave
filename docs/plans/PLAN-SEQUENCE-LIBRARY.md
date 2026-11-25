# Plan: the Sequence Library written for its styles

The factory presets are 1,032 sequences in 37 styles. Listening to the site's clips, the user met the
same figure again and again under other names ("heard earlier"), a riser under Drone, an arp with
"nothing horror about it". Counted, 544 of the 1,032 share their notes with another preset. This plan
is the overhaul: presets written for their style, not figures given a style's name and tempo.

## How the library is made

`Scripts/make_presets.py` writes every preset. About ninety are written by hand at the top (the
Phrygian stab, the acid lines, the EBM grooves). The rest come from a table of 37 `Style` lines and a
`Maker` that draws, with a generator seeded by the style's name, from tables every style shares:
kicks, backbeats, hats, bass rhythms of one bar, chord progressions, arp figures, melody rhythms.

A style's line holds its tempos, its words for names, which of the shared patterns it may draw, its
patches and at most three modes (minor, Phrygian, harmonic minor). Nothing else is the style's own:
no scale of its own, no register, no density, no way of moving. And several categories have one
figure for every style, written once in the `Maker`:

| Category | What the Maker writes for every style |
|---|---|
| Risers | 16ths up the A minor scale over 2 or 4 bars |
| Transitions | toms in 8ths then 16ths, a crash on the last |
| Impacts | kick, low tom and crash on the one |
| Drones | A and E held, the filter breathing |
| Downlifters, Sweeps, Hits, Swells | one figure each |
| Subs | six rhythms on the root |
| Stabs | "A minor stabs in a syncopated one-bar pattern", from a few rhythms |

That is the fault, and it is in the generator, so it is mended there: the presets stay generated
from a script, one edit and a rerun, checked byte for byte by `--check`.

## What changes

1. **A style gets a language.** Each `Style` line gains what makes it its own in notes and rhythm:
   its scales and the intervals it leans on, its register, how dense it is, how it syncopates, how it
   moves (static, climbing, falling, turning), and how long its phrases are. First drafts:
   - Dark techno: Phrygian and the flat second, low register, one-bar and two-bar loops that change
     little, rumble under the kick, offbeats.
   - Industrial techno, Electro-industrial, Aggrotech: tritone and minor second, metal and noise
     sources, gated and distorted, machine rhythms in threes against four.
   - EBM, Techno-body, Synth-EBM: sequenced bass in 16ths on root, octave and flat seventh, marching
     and galloping rhythms, bare fifths.
   - Horror: diminished and whole-tone sets, clusters, tritones, wide leaps, sparse and off the
     strong beats, falling lines, long holds that do not resolve.
   - Drone, Dark ambient: one or two notes held for the whole preset, slow lanes, no pulse.
   - Acid: slides and accents, octave jumps, Phrygian and pentatonic.
   - Chiptune, Menu: fast arpeggios in place of chords, major and pentatonic, high register.
   - Deep house: Dorian, sevenths and ninths, offbeat stabs, swing.
   - Psytrance: rolling 16th bass off the kick, Phrygian dominant.
   The other styles get theirs the same way. Everything stays on the 16th grid.
2. **A category gets a family of figures, not one.** A riser is a noise sweep, a pitch bend, a filter
   opening on a chord, a chord stacking a note a bar, hits doubling from quarters to 8ths to 16ths, a
   snare, clap or tom roll on the style's kit, an octave ladder, a tritone climb. Each style draws the
   ones that fit it and none is drawn by more than three styles. Transitions, impacts, drones, subs,
   stabs and pads likewise.
3. **Voices that suit the figure.** A riser on a sweep, a noise or a riser patch, never a lead by
   default; a drone on a drone or pad patch; a sub on a sub. Basic Drums' fifteen kits and its 24
   sources are used where they fit (Bunker, Machine Shop, Voltage for dark techno, industrial and
   EBM). Nothing depends on Torqwave Synth's oscillators 4 to 9.

## The order

1. The sameness test, failing, so the numbers are on the table before anything changes.
2. Risers (34 of 38 one climb), then transitions (28 of 31), impacts, downlifters, sweeps, hits.
3. Subs (26 of 45 one figure) and stabs (18 of 44).
4. Drones (11 of 22) and pads.
5. Arps, then bass lines, kicks and fills.
6. Grooves and melodies are left as they are, but a groove may move to one of the new kits.

Dark techno, EBM and industrial first inside each step.

## How many presets

Recommended: fewer. The count goes where the distinct figures are, not to 1,032. A style keeps a
preset only if its notes are its own, so the generated styles that now hold 18 to 24 with two or
three of their own will hold fewer until more is written for them. A first estimate is 850 to 900,
every one different, against 1,032 of which 488 are. The README, the guide and the site state
whatever the number is.

## What a user already has

- **Sessions.** A sequence dropped in a session, opened in the Sequencer or inserted by Ask Torq is
  loaded from its file and its notes are copied (`SequenceFile::load` at each of those places). No
  session names a factory preset, so a session is not touched by a preset that changes or goes. This
  is to be pinned by a test before the first preset changes.
- **Ratings and favourites** of factory presets are kept by the preset's path
  (`Role/Sub-role/Name.tqseq`). A rewritten preset keeps its name and its place, so its stars stay
  with it. A preset that goes leaves a line in the ratings file that is never read.
- **Kits** hold their parts' notes, and are written again by the script.
- **Ask Torq** names presets by path when it reads the library. It reads the library that is there.

## The test (`--sequences`)

One measure: what a preset plays. For drums the sounds and where they fall; for the rest the pitches
in order, the home key aside. Two strictnesses of it:

- **The same notes:** no two presets, anywhere.
- **The same shape** (the rhythm and the steps between the notes, whatever the chord): within a
  category, no shape in more than three styles. Proposed: three.
- **A floor for every style:** at least half its presets are shapes no other style has. Proposed:
  half; Chiptune stands at 1 of 24 today.
- **A preset is what it is filed as:** a drone's notes last two bars or more and none starts after
  the first beat of its span; a riser ends higher, louder, brighter or denser than it starts; a sub
  stays at or under A2; a downlifter falls; an impact sounds on the one and nowhere else.

The floors are written into the test as they are reached, category by category, so the test passes
at each commit and tightens as the work goes.

## Clips

For each category rewritten, a folder under the user's working folder with the old presets and the
new ones rendered by `TorqwaveTests`, and a list of the ones still thought weak.

## Estimate

Six to eight working days, not counting the user's listening:

| Step | Days |
|---|---|
| The test, the sessions pin, the style languages | 1.5 |
| Risers, transitions and the other effects | 1 |
| Subs and stabs | 1 |
| Drones and pads | 1 |
| Arps, bass, kicks, fills | 1.5 |
| Kits, new drum kits, README, guide, changelog, site clips | 1 |
