# Announcing Torqwave 0.19.2 on X

A long post for x.com announcing 0.19.2 and everything since 0.17.0 (0.18.0, 0.19.0, 0.19.1 and 0.19.2), in the same form as the [first announcement](ANNOUNCEMENT.md). The post itself is the plain text in the box below: copy it from there and paste it into X's composer. X does not render Markdown, so it uses plain line breaks, `•` bullets and capitals for headings, which survive pasting. It is about 3,200 characters, which needs X's long-post length (more than 280 characters).

## The post

```text
Torqwave 0.19.2 is out 🐶

A free, open-source audio editor and DAW in one, for macOS, Windows and Linux, built for sound design, writing music and working to picture. Four releases since 0.17.0. Here's everything they added.

https://github.com/BobClawblaw/torqwave

CROSSFADES (0.19.1, 0.19.2)

• Shift-X crossfades selected clips, reaching into the audio past their ends, so a split recording rejoins without a step in level
• Laid two clips over each other? Double-click the overlap: it becomes a crossfade and the editor opens
• Drag a crossfade on the lane to move the join or change its length. Right-click for Equal Gain, Equal Power or a shape for each side
• A crossfade editor: both sounds, both curves, the level they add up to, presets, and Play Out / Play In / Play Crossfade with pre- and post-roll

SOUND DESIGN IN THE SESSION (0.19.0)

• Edit in Place: double-click a clip, edit just its audio with every editor tool, and save it back as a new take
• Clip Effects: a pitch shift and an effect chain on one clip
• Combine layered clips into a new sound, and file a clip's sound in the Sound Library with its UCS category

WORKING TO PICTURE (0.19.0)

• Session timecode, read from the video too
• Find Cuts marks every new shot; Mark Hit marks a hit

MIXING AND ARRANGING (0.19.0)

• A Master track with its own effects, volume and automation
• Mixer scenes, channel-strip presets, hardware inserts, automation modes, VCA faders
• Audio that follows the tempo, comping takes, an arranger with sections

WRITING MUSIC (0.19.0)

• The Sequence Library: save a part with its instrument, effects and mix, and drop it into any session at any tempo
• The Sequencer: piano roll, drum grid with swing, controller lanes, and writing tools (chords from Roman numerals, arpeggiator, humanize, fit to scale, Vary)
• The Idea Board: launch parts and scenes on the next bar to hear what works together, then send them to the session as sections
• MIDI out, control surfaces and MIDI learn, Capture MIDI, MIDI files and packs

CUBASE-STYLE SESSIONS (0.18.0)

• A session loop, a bar grid with snap, locators, a bars-and-beats clock and Tap tempo
• An info line you can type into, a Track Inspector, and a Studio theme

ASK TORQ 🐶 (EXPERIMENTAL)

Turns a sentence into a plan of edits, with a language model you run yourself on any OpenAI-compatible server. Off by default.

• On sessions: tracks, clips, note clips, instruments, automation, effect chains, freezes and bounces
• On files: 72 edits, nearly the whole Effects menu
• In the Sequencer: writes and reviews sequences

PLATFORMS

• Mac: one universal app again, Apple silicon and Intel
• Windows: ASIO
• Plug-ins: VST3 everywhere, AU on the Mac, LV2 on the Mac and Linux

Also: plug-ins no longer crackle while the window is busy, and every fix found in use is listed in the fix log.

GET IT

• macOS 13+, Apple silicon or Intel
• Windows 10+, 64-bit
• Linux, 64-bit x86

Download: https://github.com/BobClawblaw/torqwave/releases/latest

The builds aren't notarized or signed with a developer certificate yet, so your OS will ask you to confirm the first launch.

Try it on real work and tell me what breaks: https://github.com/BobClawblaw/torqwave/issues 🙏

— Torq 🐶
```

## Notes on the claims

Everything in the post is from the [changelog](../CHANGELOG.md) and the release notes of 0.18.0 to 0.19.2. It makes no comparison with other apps, so there is nothing outside the project to check.

- **Ask Torq:** freezes and bounces each take an undo step of their own after the plan's, so the post doesn't say a plan is one undo step.
- **Testing:** the post makes no test claims. Each release's Mac suite passed in both halves of the universal app; Windows and Linux were built by the workflows and not tried by hand this round.

Before posting, check that:

- 0.19.2 is the latest release;
- the download links work on all three platforms.

## A short version

If you want a single 280-character post to go with it, or to start a thread:

```text
Torqwave 0.19.2 is out 🐶 Free, open-source audio editor + DAW for Mac, Windows and Linux. New since 0.17: crossfades with an editor, a Sequencer and Idea Board, Edit in Place, work to picture, a Master track, MIDI out and more.

https://github.com/BobClawblaw/torqwave/releases/latest
```
