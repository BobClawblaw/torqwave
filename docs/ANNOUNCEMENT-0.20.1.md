# Announcing Torqwave 0.20.1 on X

A draft, not posted. One main post and a thread of three, for the day 0.20.1 is released. Each post is the plain text in its box: copy it from there. `<release link>` stands for the release's address (`https://github.com/BobClawblaw/torqwave/releases/tag/v0.20.1` once it exists); X counts any link as 23 characters.

Check the lengths after any change:

```bash
python3 docs/compare/generate.py --posts docs/ANNOUNCEMENT-0.20.1.md
```

## The main post

One post. No picture is made for it yet: a screenshot of File > Settings... > Start with the new choice selected would suit it.

```text
Torqwave 0.20.1 is out.

New: Continue where you left off. Quit with five files open in the editor and a session in the multitrack view, and the next start brings all of it back: every file, your place in each, the session, the view you were in.

<release link>
```

## The thread

**1 of 3**

```text
Torqwave 0.20.1 is out. It is a free, open-source audio editor and DAW in one, and this release is about starting where you stopped.

Settings > Start has a new choice: Continue where you left off.

<release link>
```

**2 of 3**

```text
What comes back: every file that was open in the editor, in order, each with its selection, cursor, zoom and scroll. The effects rack. The session, with its cursor, time range and zoom. The editor or the multitrack view in front, as it was.
```

**3 of 3**

```text
It keeps a note of what was open, not a copy of your audio. A file that has moved or a drive that isn't connected is listed, the rest opens, and it is tried again at the next start.

Also a command: File > Continue Where You Left Off.

<release link>
```

## Notes for whoever posts it

- Nothing here is posted until the release is published: "is out" is true only then.
- 0.20.0's Linux package failed to build (a date format the Linux build tool could not read), and 0.20.1 fixes that. Say "for macOS, Windows and Linux" only once the 0.20.1 Windows and Linux packages are on the release page.
- What it does not bring back, if someone asks: changes you chose not to save, files never saved, undo histories, and open instrument, plug-in and tool windows. A project keeps more.
- The feature was tested by its own suite and tried once in the built app on made-up files. Say so if asked how it was checked.
- No other product is named.
