# AI assistance: the plan

Planned, not built. This is what Torqwave would do with a language model, in the order it would be built. It is on the [roadmap](../ROADMAP.md) behind the dogfooding work there.

## The first step: a language model through the OpenAI API

Torqwave gets one new setting, **Settings > AI**, off by default:

- **Provider:** Off, OpenAI, or a server of your own (a base URL). Anything that speaks the OpenAI API works: OpenAI itself with your key, or a model on your own computer or network through Ollama, LM Studio, llama.cpp, vLLM, TensorFold and the like.
- **Key:** kept in the system's keychain (Keychain on the Mac, Credential Manager on Windows, Secret Service on Linux), never in a settings file.
- **Model:** chosen from the list the server gives (`/v1/models`). No model names are built into the app.
- **Test:** one short request, showing the answer and how long it took.

Torqwave ships no model. It sends text (and pictures, for a model that reads them) to the server you choose and reads back the answer. So it carries no model's licence, and with a local server nothing leaves your computer or network. No audio is sent in this first step at all.

The plan is to build and tune it against an open-weights model on a local server first, then check it against OpenAI.

### How answers become edits

- **Commands as tools.** The model is given Torqwave's commands as tools. Torqwave already keeps a table of its 340 commands, each with a description, checked by `Scripts/check_commands.py`. The model answers with calls to those commands and their settings, and each call is checked against the table.
- **A plan before anything changes.** The steps are shown first. Preview plays the result through the rack; Apply makes the whole plan one undo step.
- **Results are ordinary.** A plan's edits, presets, markers and metadata are what you'd have made by hand, saved like anything else. Nothing stays tied to the model, and turning AI off changes nothing you have.
- **Tested without a network.** A mock server in the test suite answers with recorded replies, to check requests, tool calls, streamed answers, errors and cancelling.

## What the language model would do

All of these need only text in and text out, plus pictures for a model with vision.

1. **Ask Torqwave.** A sentence becomes a plan of commands, with settings taken from the file's own analysis. For example, "take out the hum, bring it to -16 LUFS and trim the silence at both ends".
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
