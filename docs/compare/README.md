# Torqwave against the alternatives

A running comparison of Torqwave with the editors and DAWs that are real alternatives for what it does, kept so that each release shows where Torqwave stands: where it is behind as well as where it leads.

- **[MATRIX.md](MATRIX.md):** the matrix, the coverage counts, where Torqwave is behind and ahead, prices and maturity facts, and every cell's note and source. Generated.
- **[graphics/](graphics/):** five graphics at 1600 x 900, as SVG and PNG. Generated.
  1. `1-release-by-the-numbers`: what the release adds over the one before.
  2. `2-against-the-editors`: the feature matrix against the audio editors.
  3. `3-against-the-daws-1` and 4. `4-against-the-daws-2`: the feature matrix against the DAWs, five to a graphic so the columns stay legible at X's size.
  5. `5-where-torqwave-stands`: rows covered in each area by each product, and Torqwave's trend by release.
- **[data/comparison.json](data/comparison.json):** the rows and their definitions, the products and why each is here, and the method.
- **[data/snapshots/](data/snapshots/):** one file per Torqwave release with every cell: its value, a note, the page it was read on, a short quotation or what was checked, the version assessed and the date looked up. The files together are the history the trend is drawn from.
- **[generate.py](generate.py):** makes MATRIX.md and the graphics from the data. Python's standard library only.

The announcement for a release that uses these graphics is beside the other announcements, for example [announcements/ANNOUNCEMENT-0.21.0.md](../announcements/ANNOUNCEMENT-0.21.0.md).

## Regenerating

```bash
python3 docs/compare/generate.py
```

It writes `MATRIX.md` and the five SVGs. The PNGs are written too when `rsvg-convert` is on the `PATH` (librsvg); without it, convert the SVGs with any tool you have. Nothing is installed by the script.

```bash
python3 docs/compare/generate.py --posts docs/announcements/ANNOUNCEMENT-0.20.0.md
```

counts each post in an announcement as X counts it (a link is 23 characters) and fails if one is over 280, or an alt text over 1,000.

## Rules the data keeps

- A competitor's cell comes from that product's own documentation, manual, feature or pricing page, read on the date in the snapshot, with the page's address beside it. Nothing from memory.
- A cell nobody could settle from a first-party page says `not confirmed`. It is never a guess, and it counts as neither yes nor no.
- Only what ships in the product counts. Third-party plug-ins do not.
- Torqwave's cells come from this repository's README, guide, roadmap and changelog, and say what Torqwave lacks as plainly as what it has. A release that is not out yet is described as about to be released.
- Prices carry their currency and the date looked up, and are list prices, with a sale price named as one.
- No single score or rank. Coverage is a count of rows by area; the method in `data/comparison.json` says why.
- No logos or trade dress of other products in the graphics: names as text only.

## Updating at the next release

1. **Copy the newest snapshot** to `data/snapshots/<new version>.json`. Set `release`, `previous`, `status` and `as_of`.
2. **Torqwave's columns.** The old `torqwave` cells become `torqwave_prev` (with its version and release date). Go through every row for the new `torqwave` column against the README, the guide and the changelog's entries for the release: a row changes only when the documentation says so. Rewrite `release_numbers` and `release_firsts` for the release, each number counted from the files or quoted from the release notes. When a release follows another within days and is the one being announced (0.20.1 after 0.20.0), the comparison column may stay on the older release: set `previous` to it, leave `torqwave_prev` as it is, and say why in `previous_note`, which MATRIX.md prints.
3. **Recheck the competitors.** For each product, open its release notes or what's-new page first and update `version`, `version_date` and `looked_up`. Then recheck, at least:
   - every cell that says `not confirmed`;
   - every cell the new release notes touch;
   - `price`, `licence` and `current_version`, every time;
   - the rows where Torqwave changed in this release, since those are the ones the graphics will be read for;
   - any "no" that rests on a table of contents alone (its `evidence` says what was checked), when the product had a major version since.
   A product with no new version and no price change keeps its cells and gets the new `looked_up` date only for the cells actually reread.
4. **Rows and products.** Add a row to `data/comparison.json` when a release gives Torqwave something the rows don't capture, or when a competitor's strength is missing from them; a new row needs a cell for every product in the newest snapshot (older snapshots are left as they were, so the trend's total can change: say so in the release's notes). To show a row in the editors' graphic or the DAWs' graphics, add `editors` or `daws` to its `graphics` list, and keep each graphic near twenty rows. To add or drop a product, change `products` and say why in `why` or `left_out`; a product's `matrix` names the matrix graphic it is drawn in (`matrices` lists them, with their titles and file names), and a matrix graphic holds five products beside Torqwave's two columns. More than five means another entry in `matrices`; the last graphic's number follows by itself.
5. **Run the script,** then look at each PNG: no text overlapping, nothing cut off, the date and the release's status right.
6. **Read "Where Torqwave is behind"** in MATRIX.md before writing anything about the release. The announcement must not claim more than the data shows, and must not name another product as worse.
7. **Check nothing private went in** (names, addresses of machines, home folder paths), then commit the data, MATRIX.md and the graphics together.

## What the first snapshot could not settle

The cells marked `not confirmed` are listed in MATRIX.md under "Cells not confirmed". Some "no" cells rest on a product's manual contents and feature list having no mention, not on a page that says the feature is absent; each such cell's evidence says so. Those are the first to recheck.

## 0.20.1

- 0.20.1 adds Continue where you left off, a start that reopens every file and the session. No row was added for it: a row needs a cell for each of the fifteen other products read from its own documentation, and reopening the last project at start is too common and too differently done to make a row that tells products apart. Torqwave's counts are the same as 0.20.0's.
- Its graphics set it against 0.19.2, since 0.20.1 is the release being announced and 0.20.0 came out just before it with the Mac disk image alone. 0.20.0 keeps its own snapshot and its bar in the trend.
- The other products' cells were not read again for it: they are as read on 2026-11-23.

## 0.21.0

- 0.21.0 was to be the first announced release since 0.19.2, so its graphics set it against 0.19.2; in the end it was not announced (see 0.21.1). 0.20.0 and 0.20.1 keep their snapshots, marked `"announced": false`: the graphic's panel and MATRIX.md's table of Torqwave by release show announced releases only.
- It adds a Modern type for 21 of Basic Drums' sounds, five kits and fourteen sequence presets, and corrects the Maximizer, Click and Pop Repair and Repair Clipping. No row changed its value: the rows already said yes for built-in drums, factory patterns, a limiter and click repair, so Torqwave's counts are the same as 0.20.1's (45 of 53). The notes of the drums, factory patterns and presets rows carry the new numbers.
- The other products' cells were not read again for it: they are as read on 2026-11-23.

## 0.21.1

- 0.21.0 was not announced, so 0.21.1 is the first announced release since 0.19.2 and its graphics set it against 0.19.2. 0.21.0's snapshot is marked `"announced": false` with the two before it; the panel and the table of Torqwave by release show 0.19.2 and 0.21.1.
- It makes Basic Drums a drum sampler as well as a drum synth: a Sample type on every pad with up to 16 velocity layers and round robin, 64 pads, five more effects on each pad, 54 synthesized sources, fifteen kits, eight choke groups of the user's own, and imports of a folder of sounds or a Battery 4 kit file. No row changed its value, so the counts are 0.21.0's (45 of 53):
  - **Built-in drum instrument** was yes already, since the row asks for a drum machine or a drum sampler. Its note now says what the instrument is, and that the changelog calls the sample pads, the pad effects, the new sources and the imports first working versions not yet tried by hand.
  - **Built-in sampler** stays yes on Torqwave Sampler alone: the row is about samples across the keys, which drum pads are not.
  - **Factory instrument sounds** and **Step or drum grid** stay yes; their notes carry the fifteen kits and the user pads' rows.
  - **A typed request becomes edits** stays partial: Ask Torq is still experimental, off to begin with, and ships no model. The follow-ups being measured with a real model is in its note.
  - Nothing else in the release touches a row: keeping an unsaved session at Continue where you left off and the Panic command have no row.
- No row was added for a drum sampler's depth (velocity layers, round robin, importing another product's kits): each would need a cell for the fifteen other products read from their own documentation, and this release's turn-round did not allow that reading. It is the first row to consider next time.
- The release graphic's last two tiles are now drum kits (15) and drum pads (64); audition patterns (93, unchanged) left the tiles.
- The other products' cells were not read again for it: they are as read on 2026-11-23.
