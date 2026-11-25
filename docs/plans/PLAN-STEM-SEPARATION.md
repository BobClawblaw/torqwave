# Plan: stem separation

A scope, not yet started. Asked for on 2026-11-25: splitting a finished mix into stems (vocals, drums, bass and the rest), the row of the [comparison](../compare/MATRIX.md) where Torqwave is marked partial and eight of the fifteen other products are marked yes.

## Where things stand

- **Rebalance** (Effects > Spectral > Rebalance...) turns the voice, the bass, the drums and the rest of a mix up or down. It reads the mix with masks made from signal rules: what is percussive, what is below a corner, what follows a voiced pitch. Its own header says what that is: "a reading of the mix, not a separation". It writes no stems, and a steady chord reads as voice, a kick's body as bass.
- **Dialogue Isolation** judges voiced frames the same rule-based way.
- **Nothing in Torqwave runs a neural network on audio.** The roadmap puts models that work on audio (transcription, stem separation, audio to MIDI) after Ask Torq, and [AI.md](../AI.md) sets the terms so far: Torqwave ships no model, depends on no AI company, and carries no model's licence.
- So this is not an extension of Rebalance. Separation of the quality other products have needs a trained model, and the work is in running one inside the program and in deciding where the model comes from.

## What the others do

Read from the comparison's own sources: four stems (vocals, drums, bass, other) is the common ground, run on the user's machine. Some add guitar and piano as a fifth and sixth, and one splits the drum stem into kit pieces. One says its models are Demucs-based. Of the eight marked yes, none is marked yes for Linux (one has a public beta there).

## The three decisions that shape it

### 1. Where the model comes from

This is the hard one, and it is not a technical question.

- **The best-known open model family is Demucs** (its fourth version, a hybrid of waveform and spectrogram with a transformer). Its code is under the MIT licence. Its trained weights are a different matter: they were trained on a research dataset with its own terms and on other material, the public discussion of whether the code's licence covers them is unresolved, and no explicit grant for them was found in a search on 2026-11-25. Copies on model hubs carry licence labels put there by whoever converted them.
- **Newer families** (band-split transformers) measure higher, vocals most of all. Their best weights are trained and published by individuals, each with its own terms or none stated.
- **Options:**
  - **A. Torqwave ships no weights; the user gives it a model file.** In keeping with AI.md. A Settings page names the model file, says what it must be (the format and the stems it puts out), and links to where models are published. Torqwave carries no model's licence. The cost: a first-run step most users will not take without help.
  - **B. A Download Model button** that fetches a named model from its publisher, showing its stated licence first. Easier for the user, and Torqwave then points people at weights whose terms it cannot vouch for.
  - **C. A model trained for Torqwave** on audio whose stems Torqwave owns: its own instruments can render any number of mixes with their stems, by construction. Clean rights, and months of work with an uncertain result on real recordings, which do not sound like a synth's output. Not for a first version; worth keeping as the way out if A and B prove wrong.
- **Recommended: A, with B's convenience only for models whose publisher states a licence that allows it.** Phase 0 below reads the terms properly before anything is built on them.

### 2. What runs the model

- **ONNX Runtime** (MIT licence): one library on macOS, Windows and Linux, with the graphics processor where there is one (Core ML, DirectML, CUDA) and the CPU where there is not. Takes any model exported to ONNX, so option A's "any model file" is real. Adds a dependency of some tens of megabytes to each package, and the Mac package must stay universal.
- **A port of one model to plain C++** (such ports of Demucs exist under permissive licences, built for low memory at some cost in speed). No large dependency, and it ties Torqwave to one model family.
- **An outside program or server** the user installs, which Torqwave calls. The least work and the worst experience: nothing works until Python does.
- **Recommended: ONNX Runtime,** loaded only when separation is first used, so a user who never separates pays nothing at start-up. To be confirmed in Phase 0 by building it into the three packages.

### 3. Where the stems go

- **In the editor:** Effects > Spectral > **Separate Stems...** on a file or a selection. It writes each stem as a new file beside the original (name, then the stem's name) and opens them, or, with one box ticked, opens a new session with a track for each stem, lined up, so the stems can be rebalanced, processed and bounced.
- **In a session:** right-click an audio clip, **Separate to Tracks**: new tracks under the clip's, one a stem, the original clip muted, one undo step.
- **Rebalance** gains a choice: By Rule (as now, instant) or By Model (the stems above, then the same four gains). With every gain at 0 dB the mix must still come back as it was, so the stems are made to add up to the mix.
- **Ask Torq** gets a tool for it, so "take the vocal out of this" is a plan like any other.

## What is proposed

### Phase 0: find out (two days, no feature)

- Read the terms of the candidate weights properly and write down what each allows. Decide A, or A with B.
- Export one four-stem model to ONNX, run it outside Torqwave on this Mac, and measure: seconds of work for a minute of stereo audio on the CPU and with the graphics processor, memory at its peak, and whether the stems add up to the mix.
- Build ONNX Runtime into the Mac universal package and the Windows and Linux workflows, to learn what it does to their size and build time.
- **A measure of quality that Torqwave can publish.** The usual test set cannot be redistributed. Torqwave can render its own: factory sequences on its own instruments, each track bounced alone and together, so the true stems are known. Signal-to-distortion per stem on those is a number the Quality page can show, with clips. It will flatter no model: synth mixes are not band recordings, and the page must say so.
- Ends with a yes or no, and the numbers.

### Phase 1: Separate Stems in the editor (about five days)

- The runtime, loaded on first use; the model file named in Settings, checked when chosen (its inputs, its stems, a test run on one second of audio).
- Long files in overlapping pieces, joined without a seam; any sample rate (resampled to the model's and back); mono in, stereo in; 5.1 refused with a reason in the first version.
- A background job with progress, time left and Cancel; the result is new files, so nothing is undone and nothing is lost by cancelling.
- Stems that add up to the mix, by giving what the model missed to the last stem.
- With no model chosen, the command explains what to do and offers Rebalance in the meantime.

### Phase 2: sessions, Rebalance and Ask Torq (about three days)

- Separate to Tracks on a clip, as one undo step.
- Rebalance By Model.
- The Ask Torq tool.
- The Quality page's entry: clips and the measured numbers from Phase 0's set.

### Later, each its own piece

- **Six stems** (guitar and piano), where the model has them.
- **Drums into kit pieces, and onto Basic Drums' pads:** separate a loop's drums, cut the hits, lay them on pads. Torqwave has the sampler pads and the slicing already; no other product in the comparison joins these.
- **Dialogue, music and effects** for work to picture, which is a different model and the more useful split there.
- **A model of Torqwave's own** (option C).

## What it costs the user

- **Time.** Not instant. Phase 0 measures it; expect a song to take from under a minute with a graphics processor to several minutes on a CPU alone.
- **Disk and download.** The runtime in every package, and a model file of tens to hundreds of megabytes that the user fetches.
- **Memory.** Bounded by working in pieces; measured in Phase 0.

## Risks

- **The weights' terms** (above). The whole feature rests on a model Torqwave does not own.
- **Package size and the build.** A large native library in three packages, one of them universal; the Windows and Linux packages are built by workflows that run no tests, so a broken runtime there would not be caught by them.
- **Quality claims.** A model is not a guarantee: bleed between stems, and artefacts on dense mixes. The guide and the site say what it does on Torqwave's own measured set and nothing wider.
- **Intel Macs and machines with no graphics processor** will be slow; the command says how long it expects to take before it starts.
- **The comparison** moves from partial to yes only when stems are written by a model the user can actually get.

## Tests

A tiny model made for the tests (a few fixed filters exported to ONNX) stands in for a real one, so the suite needs no downloaded weights: the pieces join without a seam, the stems add up to the mix to within rounding, sample rates and mono are handled, Cancel leaves nothing behind, a wrong model file is refused with its reason, and Separate to Tracks undoes in one step. The real model's speed and quality are measured by a command of their own, not in the suite.

## Decisions for the user

1. Where the model comes from: A (you give it a file), A with B (a download for models whose terms allow it), or wait for C.
2. Spend the two days of Phase 0 to get the licence reading, the speed and the package cost before deciding the rest?
3. Four stems first, with six later: enough?
4. Is the drums-to-pads piece worth pulling forward? It is the part no other product has.
