# AI assistance: the plan

**Experimental, being built for 0.17.0.** This is what Torqwave does, or will do, with a language model, in the order it is being built. It is on the [roadmap](../ROADMAP.md). The first version is a test: expect rough edges, check every plan before you apply it, and [tell us](https://github.com/BobClawblaw/torqwave/issues) what it got wrong.

## The first step: a language model you run yourself

Torqwave talks to a language model on a server that speaks the OpenAI-compatible API (Chat Completions with tools), the API that local model servers share: Ollama, LM Studio, llama.cpp, vLLM, TensorFold and the like. It is made for a model you run on your own computer or your own network. Torqwave gets one new setting, **Settings > AI**, off by default:

- **Server:** the server's address, such as `http://localhost:11434/v1`.
- **Key:** only for a server that asks for one. It is kept in the Mac's keychain or Windows' Credential Manager, never in a settings file. On Linux this release reads it from the `TORQWAVE_AI_KEY` environment variable.
- **Model:** chosen from the list the server gives (`/v1/models`). No model names are built into the app.
- **Test:** one short request, showing the answer and how long it took.

Torqwave ships no model and depends on no AI company. It sends text to the server you name and reads back the answer, so it carries no model's licence, and with a server on your computer or network nothing leaves it. No audio is sent at all.

It is being built and tuned against open-weights models on local servers.

### How answers become edits

- **Tools, not free text.** The model is given Torqwave's edits as tools with typed settings and ranges (below). Each call is checked before it runs, and a bad one goes back to the model with the reason.
- **A plan before anything changes.** The model works on a copy. Its steps are shown first, and Apply makes the whole plan one undo step.
- **Results are ordinary.** A plan's edits, presets, markers and metadata are what you'd have made by hand, saved like anything else. Nothing stays tied to the model, and turning AI off changes nothing you have.
- **Tested without a network.** A mock server in the test suite answers with recorded replies, to check requests, tool calls, streamed answers, errors and cancelling.

## What the model can work

The model works Torqwave through tools: functions with typed settings that Torqwave checks, runs and reports back on. Each tool calls the same code as the menu command. There are four kinds:

- **Look** tools read and change nothing: the file and selection, loudness, true peak, levels per channel, the spectrum and tonal balance, sounds, silences and clipping, markers, the session's tracks, clips, notes and automation, the plug-ins and their parameters, presets, and the user guide.
- **Try** tools render a range into a scratch copy and measure it, so the model can hear in numbers. It changes a setting, renders, and reads the loudness, peaks, spectrum and width back before it proposes anything. You can audition the same render against the original.
- **Change** tools edit. They go into the plan you see, and Apply makes the plan one undo step.
- **Ask** steps hand a choice back to you, such as picking the sound in a plug-in's own window.

### Plug-ins

Plug-ins are driven through what every host can see: their parameters and their saved state. So this works for any AU, VST3 or LV2 plug-in you have, not only Torqwave's built-in effects.

- **Find:** list the built-in effects and instruments and every scanned plug-in, with maker, category and format.
- **Build a chain:** add a plug-in to the effects rack, a track, a bus, or a track's instrument slot, then reorder, bypass or remove it.
- **Set it:** read a plug-in's parameters as it shows them ("-6.0 dB", "Hall"), with their ranges and choices, and set them by name with a value written the same way. Torqwave reads each value back and reports what the plug-in actually took, so a clamped or refused value is never hidden.
- **Presets:** load and save presets and a plug-in's own programs, and save and load whole racks.
- **Use it:** render the selection through the rack, or write automation for any parameter on a track.

What the model can't do: anything a plug-in keeps only in its own window and doesn't expose as a parameter, such as loading samples into many samplers. The plan then asks you to do that step, and opens the plug-in's window for it. Plug-ins with unhelpful parameter names ("Param 12") are taken as they are named.

### Everything else

- **The file:** selections by time, marker or sound; cut, copy, paste, crop, delete and insert; gain, normalize, loudness matching, fades and trimming; channel work; every repair tool (noise, hum, clicks, clipping, reverb, plosives, breaths); stretch and pitch; the built-in effects with their settings; tones and noise; markers, regions and metadata.
- **The session:** tracks, buses and folders; volume, pan, mute, solo and routing; importing, moving, trimming, splitting, fading and stretching clips; time edits and tempo; notes in note clips; automation; bouncing, mixdowns and stems.
- **Files:** saving and exporting in any format Torqwave writes, and batch jobs you read before they run.
- **Any other command** by name from the command table. A command that needs a dialog opens it for you.

### In 0.17.0

The first release gets the loop right on a small set of tools:

- **Look:** the file, measurements, the spectrum, finders, markers, plug-ins and their parameters.
- **Change:** selections; 30 of the Effects menu's edits with their settings (gain, normalize, loudness, EQ, compressor, limiter, de-esser, reverb, speech levelling and the rest); fades, trimming silence, silence, insert silence, crop and delete; markers and regions.
- **Plug-ins:** adding plug-ins to the effects rack, reading and setting their parameters, bypassing, moving and removing them, and rendering the rack into the audio.

It works on audio files. The session, notes, automation, Try, batch and other-command tools follow in 0.17.x, each measured against its own set of test sentences first.

### Next: the multitrack view

Ask Torq on a session works as it does on a file: the model gets a copy of the session's state, changes it with tools, and the plan is applied as one undo step (`Session::apply`).

- **The tools** (`Source/AiSession.*`): `get_session`, `list_clips` and `list_instrument_presets` look; 16 change tools cover tracks, buses and folders (add, remove, move, put in a folder), each track's volume, pan, mute, solo, output and sends, clips' places, trims, splits, gains, fades and mutes, note clips with their notes (looping if asked), an instrument track's instrument, markers and regions, and the tempo. An instrument lives in the track's chain, outside the state, so choosing one is planned as a load that runs after the plan is applied, outside its undo step. Each one calls the `SessionEdits` function the multitrack view's own command uses, with the same ranges and refusals. Tracks are named by number or name, clips by id. The plug-in look tools (`list_plugins`, `describe_plugin`) come along so the model can say what is there.
- **Instrument presets:** an instrument's sounds and kits are the presets the user saved for it (**Save Instrument Preset...**, `Source/InstrumentPresets.*`: the plug-in's whole state). Drum samplers such as Battery keep their kits in their own browsers, which no host can reach, so a kit has to be saved once before the model can choose it. `list_instrument_presets` lists them, and `set_instrument` takes one, for a new instrument or the one a track already plays. A sound that isn't saved gets no plan and a list of those that are.
- **Not yet:** a track's other effects, an instrument's own settings, replacing an instrument, automation, importing audio, bouncing and stems. A request for one of these gets no plan and a sentence saying so.
- **The UI:** in the multitrack view the Ask button starts a workspace on the session's state. Apply passes the plan's state to `Session::apply` as "Ask Torq: <request>", one undo step, and only when the session hasn't changed since the request started (the file view checks its version the same way).
- **Measured first:** `--ai-eval-set session` runs 16 requests on a test session (drums, bass, vocals and a reverb bus), each checked against the resulting state: four of them make a new instrument track, with notes, a saved kit, or a kit that isn't saved (no plan, and the saved ones named). Qwen3.8-27B got 14 of 14 in three runs out of three, and 16 of 16 in two runs once the kit requests were added.

## What the language model would do

All of these need only text in and text out, plus pictures for a model with vision.

1. **Ask Torq.** A sentence becomes a plan of commands, with settings taken from the file's own analysis. For example, "take out the hum, bring it to -16 LUFS and trim the silence at both ends".
2. **Batch from a description.** "Every file in this folder to 48 kHz WAV at -23 LUFS, with a 10 ms fade at each end" becomes a batch job you can read and change before it runs.
3. **An effect chain from a description.** "Warmer and closer", "a telephone", "a radio in the next room". The model chooses built-in effects and their settings from the file's spectrum and loudness, and saves them as a rack preset that works with AI off afterwards.
4. **Ask the guide.** Questions answered from the user guide built into the app, with a link to the page and an offer to run the command.
5. **Names and notes.** Suggested names for markers and regions, session notes, and file metadata (BWF descriptions, iXML).
6. **Sound Library categories.** Suggested UCS categories, descriptions and keywords for sounds from their names, folders and existing tags. They are never taken as the sound's category until accepted. This is item 1 on the roadmap, where word matching and naive Bayes were measured and weren't good enough. A language model would be measured against the same library the same way before it's offered.
7. **A MIDI composer.** Notes written as structured data (pitch, start, length, velocity) for a key, tempo and length. "Four bars of a walking bass under these chords", "a drum pattern for the chorus". The result lands as a note clip, editable in the piano roll.
8. **Tidying a session.** Renaming, colouring and grouping tracks, closing gaps, and evening out clip gains, as a plan of commands with checks between the steps.
9. **A second opinion on a mix.** The analysis (loudness, true peak, spectrum per track, stereo width), and for a model with vision a picture of the spectrogram or meters. It answers with specific notes, each with a one-click preset to try.

## Later: models that hear

These need models that run on audio, so they wait until the first step has been used. Each model's licence is checked before any work starts. Torqwave is AGPL-3.0, so only permissively licensed models would be offered by default, and a model with a non-commercial licence would only ever be one you add yourself. They would run in a separate worker process, as the plug-in scan does, so a model that fails can't take the editor down.

| Feature | Candidate | Licence (to confirm) |
|---|---|---|
| Transcription: markers, regions, subtitles (SRT, WebVTT), search, editing speech as text | whisper.cpp, or a server's `/v1/audio/transcriptions` | MIT |
| Stem separation onto session tracks | HT-Demucs through ONNX Runtime | MIT |
| Audio to MIDI, into a note clip | Basic Pitch | Apache-2.0 |
| Voice noise reduction | DeepFilterNet, RNNoise | MIT/Apache, BSD |
| Search the Sound Library by meaning ("metal door slam in a big room") | CLAP audio embeddings | to check |
| Scratch voice-over from text | Kokoro | Apache-2.0 |
| Music from a description, or a selected range regenerated in place | ACE-Step 1.5, through ComfyUI on a GPU machine | Apache-2.0 |
| Sound effects from a description | Stable Audio, through ComfyUI | to check |
| Foley in sync with a video | MMAudio | code MIT; weights to check |

## Rules

- AI is off by default, and Torqwave is complete without it.
- Local first: audio leaves the computer only once that is turned on, and each action that would send it asks first.
- Keys live in the system keychain, never in settings, logs, sessions or crash reports.
- Every feature is measured before it ships, like the rest of Torqwave: an evaluation set per job (sentences with the plans expected, sounds with known categories), and the results in the changelog.
- Generated audio says so in its metadata (BWF/iXML): that it was generated, by which model, from which prompt.
