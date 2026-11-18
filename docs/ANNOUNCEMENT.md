# Announcing Torqwave on X

A long post for x.com introducing Torqwave. The post itself is the plain text in the box below: copy it from there and paste it into X's composer. X does not render Markdown, so it uses plain line breaks, `•` bullets and capitals for headings, which survive pasting. It is about 3,200 characters, which needs X's long-post length (more than 280 characters).

## The post

```text
Introducing Torqwave 🐶

A free, open-source audio editor for macOS, Windows and Linux, growing into a real alternative to Audacity.

Audacity 4.0 shipped without its note tracks and its mixer. Torqwave has both, plus instrument plug-ins, buses and sends, automation, surround and video.

github.com/BobClawblaw/torqwave

FIRST, A NEW NAME

If you saw this project as SpectraBlade: same app, new name. Another audio product already used a similar name, so as of 0.16.0 it's Torqwave. Nothing is lost in the move. Torqwave opens SpectraBlade's sessions, projects, racks and presets, and copies its settings and Sound Library across the first time it starts.

WHAT IT IS

A clean-room editor built in C++20 on JUCE, licensed AGPL-3.0. Waveform and spectrogram editing, repair and loudness tools, 20+ built-in effects, multitrack sessions with mixdown, and VST3 plug-ins (plus Audio Units on the Mac, and LV2 on the Mac and Linux). The user guide is built in: press F1.

WHAT YOU GET THAT AUDACITY DOESN'T HAVE

• Instrument plug-ins and MIDI: instrument tracks that host VST3, AU and LV2 synths, a piano roll, note clips, live MIDI input and an on-screen keyboard. Audacity can't host instruments at all, and Audacity 4.0 shipped without its note tracks.
• A real mixer: buses, sends, sidechain inputs on plug-ins, ducking under a voice, track folders and groups. Audacity has no buses or sends, and 4.0 shipped without its Mixer.
• Real automation: pan, effect and instrument parameters, not just volume, drawn as envelopes or written live from the controls as the session plays. Audacity's envelopes only set volume.
• Surround: 5.1 and 7.1 tracks with a surround panner.
• Video: a Video panel that follows the playhead, and the edited sound or session mix written back into a copy of the video.
• A Sound Library: your sound-effects folders indexed and searchable, browsed by UCS category, rated and tagged.
• Spectral repair by hand: box, lasso and brush selections on the spectrogram, and Heal to fill a spot from the sound around it.
• Loop recording into takes, punch-in with pre-roll, and input monitoring through a track's effects.
• Docked, tabbed or floating panels kept in workspaces, nine themes, and keyboard shortcuts you can search and remap (with an Audition-style key set).

STILL EARLY, AND THAT'S THE POINT

Torqwave is still early in development, but I'm happy enough with it to put it into more hands for testing. Every release is a dogfooding release: what real use turns up gets fixed and shipped, and 1.0.0 comes once a round of use like that runs clean.

The latest releases came straight out of that loop. Since 0.16.1: the user guide inside the app, a home screen with your recent work, ripple editing for clips and time ranges, Option-drag to copy clips, envelope points you can select and delete, and automation that shows itself, on instrument tracks too. 0.16.4 is the newest.

GET IT

• macOS 13+ on Apple silicon
• Windows 10+, 64-bit
• Linux, 64-bit x86

Builds are on the GitHub Releases page. They aren't notarized or signed with a developer certificate yet, so your OS will ask you to confirm the first launch.

Try it on real work and tell me what breaks: issues and ideas are welcome on GitHub. 🙏

— Torq 🐶
```

## Notes on the claims

Every comparison with Audacity above was checked on 2026-11-18:

- **Audacity 4.0** came out on 3 September 2026, rebuilt on Qt. Its own release notes list what is not in it at launch, among them Note/MIDI tracks and the Mixer: [Audacity 4.0.0 release notes](https://www.audacityteam.org/changelog/4.0.0/), with coverage in [OMG! Ubuntu](https://www.omgubuntu.co.uk/2026/09/audacity-4-released).
- **Instrument plug-ins:** Audacity hosts effect plug-ins but not instruments (VSTi); requests for it remain open on the [Audacity forum](https://forum.audacityteam.org/t/midi-support-w-vst-instruments/105529).
- **Buses, sends, sidechains, effect automation, surround panning, a video panel and a sound library** are not in Audacity 3.7 or 4.0.
- **Envelopes:** Audacity's Envelope Tool shapes a track's or clip's volume only; it has no pan, effect or instrument parameter automation. The post claims pan, effect and instrument automation, not volume envelopes, as Torqwave's own.
- **LV2:** Audacity loads LV2 effects too, so the post lists LV2 in what Torqwave is, not in what Audacity lacks.
- Audacity 4.0 does have **workspaces** and **non-destructive clip editing**. The post doesn't claim either as Torqwave's alone.

What Torqwave has is from the [README](../README.md) and the [changelog](../CHANGELOG.md). Before posting, check that:

- the latest release is the one named in the post;
- the download links work on all three platforms.

## A short version

If you want a single 280-character post to go with it, or to start a thread:

```text
Introducing Torqwave: a free, open-source audio editor for macOS, Windows and Linux, and a growing Audacity alternative (formerly SpectraBlade). Instrument plug-ins, piano roll, buses, automation, surround, video. Early, but ready for testers.

github.com/BobClawblaw/torqwave
```
