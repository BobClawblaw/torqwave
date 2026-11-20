# Ask Torq 🐶 (experimental)

**Experimental.** Ask Torq is new in 0.17.0 and is a test of the idea. It can misread what you ask, pick settings you wouldn't, or stop partway. Read every plan before you apply it, listen afterwards, and [tell us](https://github.com/BobClawblaw/torqwave/issues) what it got wrong.

Ask Torq turns a sentence into edits: "trim the silence at both ends, bring it to -16 LUFS and fade out the last second". A language model reads your sentence, looks at the file through Torqwave's measurements, and makes a plan with Torqwave's own edits. Nothing changes until you read the plan and press Apply, and Apply is one undo step.

## What it needs: a model you run yourself

Torqwave ships no model. It talks to a model server that speaks the OpenAI-compatible API, the API local model servers share: [Ollama](https://ollama.com), [LM Studio](https://lmstudio.ai), [llama.cpp](https://github.com/ggml-org/llama.cpp)'s `llama-server`, [vLLM](https://github.com/vllm-project/vllm) and the like, on your computer or another one on your network.

The model has to support tool calling (function calling). Models that do it well are the larger open-weights instruct models; small ones tend to call the wrong edit or give up. It was built and measured with a 27-billion-parameter model (see the [changelog](../../CHANGELOG.md) for the results).

What goes to the server: your sentence, the file's name, and the numbers Torqwave measures (lengths, levels, loudness, the plug-ins you have and their settings). **No audio is ever sent.** With a server on your own computer or network, nothing leaves it.

## Setting it up

1. Start your model server and load a model that supports tools.
2. Open **Settings > AI**.
3. Press **Add** beside **Endpoint**, and give the server a **Name** you'll know it by. Then, under **Server**, type its address, such as `http://localhost:11434/v1` for Ollama on this computer, or `http://studio-pc.local:8000/v1` for a server elsewhere on your network. An address without a path gets `/v1`.
4. **Key:** leave it empty unless your server asks for one. A key you save is kept in the Mac's keychain or Windows' Credential Manager, never in Torqwave's settings. On Linux, this release reads it from the `TORQWAVE_AI_KEY` environment variable instead. After you install a new version on the Mac, macOS may ask whether Torqwave may use the key it saved: choose Always Allow.
5. Press **List Models** and choose the model.
6. Press **Test**. Torqwave asks the model for a short reply and shows it, with how long it took.
7. Turn on **Ask Torq**.

**More than one server.** Settings > AI keeps a list of endpoints: a model on this computer, say, and a bigger one on another machine on your network. Each keeps its own name, address, model and key. Choose one under **Endpoint** to use it; Ask Torq and the plug-in catalogue use the one chosen. **Add** puts another in the list. **Delete** removes the one shown, with its saved key unless another endpoint in the list has the same address. Settings from an earlier version come back as an endpoint named "Server 1".

## Asking

With an audio file open, click **Ask Torq 🐶** at the top of the window (it shows while Ask Torq is turned on, in the editor; the multitrack view has no Ask Torq yet), or choose **Edit > Ask Torq 🐶...**. Type what you want and press Return or **Ask**.

The window shows the model at work: its thinking (for a model that shows it), each tool it calls, and each step it adds to the plan. When it's finished, the plan is listed. **Apply** makes the changes as one undo step, named after your request; **Discard** throws the plan away. If the file changes while the model works, the plan isn't applied: ask again.

**Stop** cancels a request in progress.

Things to try:

- "Trim the silence off both ends."
- "Make the whole file -20 LUFS integrated."
- "Fade out the last half second of the sound."
- "Add a region named Verse over the part with sound in it."
- "Put the built-in Compressor in the effects rack with a 4:1 ratio and a threshold of -24 dB."
- "Raise the gain by 6 dB, then limit it so the peaks stay under -6 dB."
- "Normalize each channel on its own to -3 dB."

## What it can do

It works on the audio file that's open and, in the multitrack view, on the session (experimental).

- **Look:** the file's length, rate and channels, the selection and markers; loudness, true peak and each channel's levels; the spectrum in octave bands; where the sounds, silences and clipping are.
- **Edit:** select; 30 of the Effects menu's edits, with their settings checked against the same ranges as their dialogs (gain, normalize, Match Loudness, EQ band, compressor, limiter, maximizer, de-esser, de-plosive, breath control, Level Speech, Reduce Reverb, reverb, echo, chorus, flanger, phaser, tremolo, stereo width, balance, stretch, pitch and more); fades; trimming silence; silence, delete, crop and insert silence; adding, renaming and deleting markers and regions.
- **Plug-ins:** list the built-in effects and the plug-ins Torqwave found on your computer, read what the plug-in catalogue knows of them, add them to the effects rack, read and set their parameters, bypass, move and remove them, and render the rack into the audio.

- **Sessions:** tracks, buses and folders, volume, pan, mute, solo, outputs and sends; moving, trimming, splitting and fading clips, their gain, and comping their takes; note clips with their notes; an instrument track's instrument and its saved presets; a track's effects, added at the end of its chain; volume and pan automation; markers and regions; the arranger's sections (adding them, moving, copying and deleting them with their time, making them whole bars longer or shorter, and the arrangement chain), so "double the chorus" or "make the intro 4 bars shorter" work; and the tempo. Apply makes the plan one undo step.

Plug-ins are set through their parameters, the way automation sees them, so any AU, VST3 or LV2 effect works, not only Torqwave's own. The model writes values the way the plug-in shows them ("-12 dB", "4:1", "Hall"), and Torqwave reports back what the plug-in actually took. What a plug-in keeps only in its own window (loading samples, some routing) is out of the model's reach.

### The plug-in catalogue

So that it can choose a plug-in without opening each one to look, Ask Torq keeps a catalogue of your effects: each one's parameters (names, units, ranges, choices and defaults) and presets' names, and a short description of what it does, what it's good for and which settings matter, written by your model. The plug-in list it reads shows each description's first sentence, and it can read a whole entry before adding anything.

**Settings > AI > Plug-in Catalogue...** opens it:

- **Learn All** goes through every effect Torqwave knows. It reads each one's parameters in the same separate process as the plug-in scan, so a plug-in that crashes or hangs takes only that process with it and is marked as couldn't be opened. Then the model describes it from those parameters and what it knows of the plug-in. An AU and a VST3 of the same plug-in share one description. Learning goes on in the background with the window closed, and **Stop** halts it; the next Learn All goes on from where it stopped. Without a model set up, it only reads the parameters.
- **Torqwave ships descriptions** of 406 effects (Apple's, iZotope, Plugin Alliance, Native Instruments, Soundtoys, Kilohearts and others, and its own), written by a model and then reviewed, so many of yours are described before you learn anything. Learn All then only reads their parameters and asks your model about the rest. A description you write or learn yourself is used instead of the shipped one.
- How long it takes depends on the model: each description is one request. Qwen3.8-27B on a local server took about 10 seconds each.
- A plug-in the model doesn't know is marked **not known** rather than guessed at, and Ask Torq is told so.
- **Choose a plug-in** to read its entry. Edit it and **Save** to keep your own text, which learning never replaces. **Forget** drops what is known of the plug-in, so the next Learn All reads and describes it again (or, for one Torqwave ships a description of, brings that back).
- The catalogue is kept as `Plug-in Catalogue.json` in the Torqwave folder of your user library (on the Mac, `~/Library/Torqwave`), beside the Sound Library's index. A plug-in's parameters are read again when it is updated; its description is kept.
- Instruments aren't learnt yet; they will be when Ask Torq works on sessions.

The model's descriptions can be wrong, as anything it writes can, especially for a plug-in it doesn't know. Reading a few is worth it, and a wrong one is easily edited or forgotten.

Changes to the effects rack are applied with the plan but aren't part of its undo step, as rack changes never are. A rack the plan rendered into the audio comes back to the rack when you undo, as after Apply Effects Rack.

## When it goes wrong

- **"Couldn't reach" the server:** check the address and that the server is running. From another computer, the server must listen on the network, not only on `localhost` (Ollama: `OLLAMA_HOST=0.0.0.0`). On the Mac, a server on another computer also needs Torqwave allowed under System Settings > Privacy & Security > Local Network; macOS asks the first time.
- **It answers but never edits:** the model may not support tool calls, or the server may need tool calling turned on (llama.cpp: `--jinja`).
- **It stops partway or keeps calling tools:** after 60 rounds of tool calls it is asked to finish, and the plan it made so far is shown, marked as possibly incomplete; you can still apply or discard it. Asking for the same thing a third time with nothing changed is refused, which breaks most loops. For a long job, ask for it in parts, or try a larger model.
- **It's slow:** a model that thinks before it answers can take a minute or more for each step. The window shows the thinking as it goes.

The plan for what comes next, including sessions and models that hear, is in [AI assistance](../AI.md).
