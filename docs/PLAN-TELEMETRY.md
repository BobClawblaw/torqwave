# Plan: optional reports (usage and bug reports) to a private GitHub repository

This is a design, not yet agreed and not built. Nothing here exists in the app. Steps that need an account, a token or a hosted service are marked **(needs the developer)** and wait for a yes.

Torqwave sends nothing anywhere today, apart from Ask Torq's requests to the server the user names. This plan adds two kinds of report, both off until the user turns them on:

- **Bug reports:** what went wrong, with the app's own log, shown to the user before it is sent.
- **Usage reports:** once a week, which version and system, and how often each command and built-in instrument or effect was used.

Both end up in a private GitHub repository that only the developer can read.

Each piece below has what it is, what it builds on in the code today, its size (**S** is days, **M** about a week, **L** more) and how it is tested.

## Is this reasonable?

Yes, with one condition: **a small relay has to be hosted.** There is no sound way for an open-source app to write to a private repository directly.

- The app's source and binary are public, so any GitHub token put in either is public too. A token that can write to the private repository can also read it. This rules out every design without something in between.
- The something in between can be very small: one serverless function of about 200 lines, on a free plan, holding one secret. At the scale of a one-person app it should cost nothing to run. It is one more thing to keep working, and one privacy note to keep true.
- The app's side is ordinary work: about three to four weeks in all, in steps that each ship alone.

**The smallest version worth shipping needs no relay, no ID and no account:** Help > Report a Problem... builds a report with private things taken out, shows it, and lets the user copy or save it and open the public issue page or send it some other way (steps 1 to 3). It is useful on its own, and everything after it reuses its report and its redaction.

**What to expect from it.** Bug reports are the valuable part. Usage numbers from an opt-in switch in a small app will be few and will lean towards keen users; they answer "is anyone using this at all" and "which versions and systems are out there", not much more. The plan builds usage last for that reason.

## What the code gives us to build on

- **The log** (`Source/Main.cpp`). `juce::FileLogger::createDefaultAppLogger ("Torqwave", "Torqwave.log", ...)` writes one log file: under the user's Logs folder in `Torqwave/` on macOS, and in the app data folder's `Torqwave/` on Windows and Linux. What is written to it today:
  - a first line with the app's name and version (and JUCE's own start line with the date);
  - `Asking: <title>: <message>` for every question box and `Error shown: <title>: <message>` for every error box (`askThreeWay` and `showError` in `Source/MainComponent.cpp`). `showError` has over 200 callers, and many messages carry a file's name or its full path;
  - the audio device: its name, sample rate and buffer size, or "none";
  - plug-in scans: started, the summary, and for each plug-in that fails its file path or identifier and the reason (`Source/PluginScanner.cpp`);
  - `Could not reopen <full path>: <error>` for the last file or session;
  - errors opening a control surface or track inputs, which can name MIDI and audio devices;
  - recording: input channels, counts of tracks and frames;
  - `Could not write the recovery copy: <error>`;
  - after a crash, `Crashed. The stack:` and a stack trace.

  So the log holds full paths (with the user's account name in them), file and session names, plug-in paths and device names (which are often a person's name). It does not hold audio, Ask Torq's prompts or answers, or keys. **It cannot be sent as it is.**
- **Crash handling** (`Source/Main.cpp`). `juce::SystemStats::setApplicationCrashHandler` writes the stack to the log. Nothing is uploaded and nothing is shown at the next start.
- **The recovery copy** (`Source/SessionRecovery.h`, `keepRecoveryCopy` and `offerRecoveredSession` in `MainComponent.cpp`). A copy of an unsaved session is written every 120 seconds and offered at the next start. This is the one place where the app already knows that the last run did not end normally, and where a question at start-up already exists.
- **Commands** (`MainComponent::perform`, `getAllCommands`, `getCommandInfo`; the `ApplicationCommandManager` is `commands`). Every menu item, key and button that is a command passes through `perform`, which already looks each one up for Repeat Last. One line there counts usage. Command IDs are a plain `enum` (`cmdAbout`, `cmdMixDown` and so on), so **their numbers change when a command is added** and can't be sent; reports need a fixed name for each. `Scripts/check_commands.py` already reads the command table from the source for the test suite.
- **Settings** (`MainComponent` constructor). `juce::ApplicationProperties` with `applicationName` "Torqwave", suffix `settings`, folder `Torqwave`: one settings file in the app data folder (`Application Support/Torqwave` on macOS, `%APPDATA%\Torqwave` on Windows, `~/.config/Torqwave` on Linux). Presets, drum maps, templates and the recovery copy live in the same folder (`Source/EffectPresets.cpp`, `Source/DrumMaps.cpp`). `PreviousName::copyData` once copied a whole settings folder across, which is how copied settings behave.
- **The Settings window** (`Source/SettingsPanel.h`). Sections of rows (switch, choice, number, button, or a component), each reading and writing through functions. A new section is a list of rows.
- **HTTP** (`Source/AiClient.cpp`, `Ai::Client::open`). A POST of JSON through `juce::WebInputStream` with headers and a time limit, called from a background thread, with cancel. On Linux it goes through libcurl, loaded only when first needed (`NEEDS_CURL`, `JUCE_LOAD_CURL_SYMBOLS_LAZILY`), so the app starts without it. The tests have a mock HTTP server (`MockServer` in `Tests/AiTests.cpp`) that answers with recorded replies.
- **Secrets** (`Source/AiKeychain.h`): the system's keychain for Ask Torq's keys. Reports need no secret in the app, so this is not used.
- **The build stamp** (`Source/BuildStamp.h`): whether this is a release build, and for a test build its time and commit.
- **The fix log** ([DOGFOODING.md](DOGFOODING.md)): every problem found by use gets a row and a GitHub issue, in the commit that fixes it. Reports from users should end as rows there.

## How a report reaches a private repository

### The options

| Option | What it needs | Good | Bad |
|---|---|---|---|
| **A token in the app** | nothing | none | The token is public the day it ships. Never. |
| **No network: copy, save, or the public issue page** | nothing | No infrastructure, no ID, the user sees and sends every word. Works today. | The user has to do something, and needs a GitHub account for the issue page. Few reports. No usage numbers. |
| **A relay with a fine-grained token** | a serverless function and one secret | Small. The token is limited to one repository and two permissions. | The token belongs to the developer's account and must be renewed if it has an expiry. |
| **A relay with a GitHub App** | the same function, a GitHub App and its private key | Tokens last an hour and are made as needed. Reports come from a bot, not the developer's account. | More to set up: the function has to sign a token request with the App's key. Same permissions on the same repository, so a leak costs the same. |
| **Relay sends `repository_dispatch`, an Action commits** | the relay, plus a workflow | The relay's token needs no knowledge of the layout. | One workflow run for each report. Runs on a private repository are paid from a monthly allowance, so a few dozen reports a day would use it up. A payload is limited to 64 KB and 10 top-level properties. |
| **A hosted crash or analytics service** | an account with a third party | Symbolication and grouping done for you. | The data sits with a third party, not in the repository. An SDK in the app. Against this plan's own rules (see Risks). |
| **Email** (`mailto:`) | nothing | No infrastructure. | Shows the user's address to the developer. No attachment through `mailto:`. |

### Recommended

**A relay with a fine-grained token first; move to a GitHub App later if renewing the token becomes a chore.** The app talks only to the relay. The relay is the only thing that talks to GitHub.

```
Torqwave  --HTTPS POST (JSON, no secret)-->  relay  --GitHub API (token)-->  private repository
                                               |
                                               +-- a small key-value store: counters, the day's usage rows
```

- **Bug reports become issues.** One POST makes one issue through the issues API, or one comment on the issue that already has the same signature.
- **Usage reports become one file a day.** The relay does not write to GitHub for each one. It keeps the day's rows in its store and, on a daily timer, writes them as one file through the contents API. This keeps commits to one a day, keeps well inside GitHub's limits, and avoids two writes to one branch at once.
- **The relay's source lives in the public repository** (`Relay/`), so anyone can read what it does with a report. Its secret and its address's configuration do not.
- **Where it runs** is the developer's choice (see Decisions). The design assumes a serverless platform with a free plan, a timer and a small store; Cloudflare Workers is the example the numbers below were checked against.
- **The address** should be a host name the developer controls (for example `reports.<the project's domain>`), so the relay can move without a new release. A platform's default address works too but ties every shipped version to that platform, and its name must not contain anything private.

### GitHub facts this relies on

Looked up on 2026-11-22 in GitHub's documentation:

- **Rate limits** ([REST API rate limits](https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api)): 5,000 requests an hour for a personal token, and at least 5,000 for a GitHub App installation. Requests that create content are also limited to 80 a minute and 500 an hour. The relay's own caps (below) sit far under these.
- **Fine-grained token permissions** ([permissions list](https://docs.github.com/en/rest/authentication/permissions-required-for-fine-grained-personal-access-tokens)): creating an issue or a comment needs **Issues: write**; writing a file needs **Contents: write**; `repository_dispatch` also needs Contents: write. A fine-grained token can be limited to chosen repositories ([managing tokens](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/managing-your-personal-access-tokens)); it always has read access to public repositories as well.
- **The contents API** ([repository contents](https://docs.github.com/en/rest/repos/contents)): writes to it must be made one after another, not in parallel. This is why usage is written by one timer, not by each request.
- **`repository_dispatch`** ([create a repository dispatch event](https://docs.github.com/en/rest/repos/repos#create-a-repository-dispatch-event)): the payload must be under 64 KB with at most 10 top-level properties.
- **GitHub App tokens** ([installation access tokens](https://docs.github.com/en/apps/creating-github-apps/authenticating-with-a-github-app/generating-an-installation-access-token-for-a-github-app)): expire after one hour and can be limited to chosen repositories and permissions.
- **Actions on a private repository** ([billing for GitHub Actions](https://docs.github.com/en/billing/concepts/product-billing/github-actions)): the page now states the free plan's allowance as an amount of money a month (12 US dollars when read), with a Linux runner at 0.006 dollars a minute, which comes to about 2,000 Linux minutes. The weekly rollup below needs a few minutes a month.
- **The relay's platform** ([Cloudflare Workers limits](https://developers.cloudflare.com/workers/platform/limits/)): the free plan allows 100,000 requests a day and 10 ms of processor time for each, which a validate-and-forward function fits. Its rate limiting ([rate limit binding](https://developers.cloudflare.com/workers/runtime-apis/bindings/rate-limit/)) counts per location and is approximate, and Cloudflare advises against using an IP address as the only key.

Not confirmed from GitHub's own documentation:

- The longest issue body. Error messages reported by many projects say 65,536 characters. The relay caps a report's log well below that (48,000 characters).
- Whether the free plan's Actions allowance has changed in kind or only in how it is written.
- The free limits of the platform's key-value store, and whether its timers and rate limiting are on the free plan. These decide the choice of store, not the design.
- How a GitHub App's private key has to be converted to be used by a serverless function's signing API.

### Abuse and cost control

Anyone can read the app's source and call the relay. The relay assumes every request may be hostile.

- **Shape first.** A request must be a POST of JSON, under 64 KB for a bug report and 8 KB for a usage report, and match the schema exactly: known fields only, each of the right type and length, version strings and system names matching a fixed pattern, counters as small whole numbers, command names from the characters `A-Z a-z 0-9` only. Anything else gets `400` and is dropped without being stored.
- **Limits per ID:** one usage report a day and five bug reports a day.
- **Limits per address:** a short-lived counter (ten requests a minute) on the sender's IP address, held in memory only. The address is never written to the store, the log or the repository.
- **Limits in all:** at most 30 new issues an hour and 200 a day, and 5,000 usage rows a day. Past a cap, reports are dropped and the relay answers `429`. One issue named "Relay: cap reached" is opened so the developer knows.
- **A kill switch at the relay:** a setting that makes it answer `410 Gone` to everything. The app, on `410`, stops sending for 30 days and tries again after. A second setting does the same for one kind of report or for versions below a given one.
- **A switch in the build:** the relay's address is a build setting (`TORQWAVE_REPORTS_URL`) that is empty unless the release workflows set it. Test builds, and forks built from the source, send nothing and show the sending controls disabled, with Copy and Save still working.
- **What reports contain is untrusted.** The relay puts a report's text inside a code block and removes `@` so nothing in it can mention anyone or be read as formatting. A report is data: nothing in one is ever followed as an instruction, by a person or by a tool reading the repository.
- **When the relay is down, slow or refusing:** the app does nothing the user can see. Sending happens on a background thread with a 10 second limit. A usage report that fails is kept and tried once more the next day, then dropped. A bug report the user sent by hand shows one line, "Couldn't send. Copy or save it instead.", and nothing else. The app never waits for the relay at start-up or at quit.
- **Linux without libcurl:** sending is skipped, as Ask Torq is; Copy and Save work.

## The machine ID

**What it is:** 128 bits from the system's own random source, written as 32 hex characters. It is made at the moment the user first turns a switch on, never before.

**What it is not made from:** hardware serials, MAC addresses, disk or system IDs, the user's name, the computer's name, the install path, or the time. Nothing about the machine or the person can be worked out from it.

- **Check before use:** `juce::Uuid` may mix the machine's network addresses into its seed in some JUCE versions. This could not be read here (JUCE's source is not in the checkout). Use the system's random source directly (`SecRandomCopyBytes`, `BCryptGenRandom`, `getrandom`) so the question does not arise.

**What it lets the developer do:**

- count how many installs sent reports in a week;
- see that five crash reports came from one install, not five;
- drop a report sent twice;
- find and delete everything one install sent, when asked.

**What it must not allow:** finding out who someone is. The relay stores no IP address, the reports carry no names, and the ID is never sent anywhere except with a report. It is not shown in the title bar, not written to the log, not put in session files, and not sent to Ask Torq's server.

**Where it is kept:** a file of its own, `Reports.xml`, in the app data folder beside the settings file. It holds the ID, the two switches, the date and wording version of the consent, and the usage counters waiting to be sent. It is kept out of the main settings file so that a settings file given to someone else, or attached to a report, never carries it.

| Platform | Folder |
|---|---|
| macOS | `Library/Application Support/Torqwave` in the user's home |
| Windows | `%APPDATA%\Torqwave` |
| Linux | `~/.config/Torqwave` |

**How it behaves:**

- **Each user account has its own.** Two accounts on one computer are two IDs. "Per machine" really means "per install, per account".
- **Reinstalling keeps it**, since installers and uninstallers leave the app data folder alone. Deleting the folder removes it.
- **Copying the whole folder to another computer copies it.** Two computers then count as one. This is accepted: telling them apart would need something from the hardware. The ID can be reset on either.
- **Reset** (a button in Settings) makes a new one. Reports already sent stay under the old one and can no longer be tied to this install.
- **Turning both switches off deletes it,** with the counters. Turning one on again later makes a new one.
- **A report sent by hand with both switches off carries no ID at all.**

## What is collected

### Usage report

Sent at most once a week, for the seven days before. Counters are kept in `Reports.xml` and cleared when sent.

```json
{
  "schema": 1,
  "kind": "usage",
  "id": "5f0c2a9e41d7b3806c1e9a47d2f0b815",
  "app": { "version": "0.20.0", "build": "release" },
  "system": { "os": "macOS", "osVersion": "15.1", "arch": "arm64" },
  "week": "2026-W47",
  "sessions": 9,
  "daysUsed": 4,
  "minutesOpen": 340,
  "commands": { "MixDown": 3, "Normalize": 12, "AddInstrumentTrack": 5 },
  "builtIns": { "TorqwaveSynth": 6, "BasicDrums": 2, "ParametricEq": 14 },
  "plugins": { "vst3Loaded": 4, "auLoaded": 1 },
  "askTorq": { "on": true, "requests": 7 }
}
```

- `build` is `release` or `test`; a test build's commit is not sent.
- `osVersion` is major and minor only. `arch` is the app's, plus `rosetta: true` when translated.
- `minutesOpen` is rounded to ten.
- `commands` holds the fixed name of each command used and how often, capped at 9,999. A command not used is absent.
- `builtIns` counts Torqwave's own instruments and effects by a fixed name, once for each time one is added.
- `plugins` holds **counts only**. No third-party plug-in is named in a usage report.
- `askTorq` says whether it is on and how many requests were made. Nothing about the server, the model or what was asked.

**Never in a usage report:** file, session, track, clip, preset or marker names; paths; plug-in names; device names; anything typed; durations of the user's audio; the time of day; the language or region; screen size.

### Bug report

```json
{
  "schema": 1,
  "kind": "bug",
  "id": "5f0c2a9e41d7b3806c1e9a47d2f0b815",
  "app": { "version": "0.20.0", "build": "release" },
  "system": { "os": "Windows", "osVersion": "11.0", "arch": "x86_64" },
  "what": "crash",
  "signature": "c41e77a0",
  "top": "SessionMixer::render",
  "stack": [ "SessionMixer::render", "AudioEngine::audioDeviceIOCallback", "..." ],
  "lastCommands": [ "AddTrack", "Record", "Stop", "MixDown" ],
  "audio": { "sampleRate": 48000, "buffer": 256, "outputs": 2, "inputs": 2 },
  "plugins": [ { "name": "kHs Gain", "format": "VST3", "version": "1.2.0" } ],
  "note": "It closed when I pressed Mix Down while the take was still recording.",
  "log": "Torqwave 0.20.0\nAudio device: <device>, 48000 Hz, 256 samples\nError shown: Mix Down: Couldn't write <path>.\nCrashed. The stack:\n..."
}
```

- `what` is `crash`, `error` (an error box the user chose to report) or `manual` (Report a Problem).
- `signature` is the first 8 hex characters of a hash of the app's version and the top five stack frames, with addresses and offsets removed. For an error it is a hash of the error box's title and its message with every private part replaced.
- `stack` holds function names and module file names (no folders). On a build without symbols it holds the module and offset, which the developer resolves with the symbol files kept for each release (step 5).
- `lastCommands` is the last 20 command names, with no times.
- `plugins` lists the plug-ins loaded in the session, by name, format and version. **A tick box in the report window, "Include plug-in names", turns this into counts.** It starts ticked for a crash (a plug-in is often the cause) and the user can untick it.
- `note` is what the user typed in the report window, sent as typed. The window says so.
- `log` is the last 200 lines of the log after redaction, at most 48,000 characters.
- `id` is absent when both switches are off.

**Never in a bug report:** audio or any part of it; session files or the recovery copy; file, session, track or clip names; full paths; Ask Torq's prompts, answers, server addresses or keys; the settings file; the user's or the computer's name; device names; screenshots; an email address (unless the user types one into the note).

### Redaction

Two layers, because the second alone would be guesswork.

**At the source.** A small logging helper replaces direct `juce::Logger::writeToLog` calls. Private values are passed to it marked: `Log::path (file)`, `Log::name (text)`, `Log::device (name)`. The log file on disk still holds the real value (the user reads their own log), wrapped in two marker characters. The report builder replaces every marked span. `showError` and `askThreeWay` take the same marked pieces, so their 200-odd callers are converted a group at a time.

**A backstop over every line**, for anything not marked yet and for the stack trace:

1. The home folder's path becomes `~`; then any remaining absolute path (`/...`, `X:\...`, `\\server\...`) becomes `<path>`.
2. The account's name and the computer's name, as whole words in any case, become `<user>` and `<host>`.
3. A word ending in an audio, session, preset or plug-in extension (`.wav`, `.aiff`, `.flac`, `.mp3`, `.m4a`, `.ogg`, `.tqsession`, `.tqseq`, `.tqkit`, `.vst3`, `.component` and the rest of the app's list) becomes `<file.ext>`, keeping the extension.
4. URLs become `<url>`; IPv4 and IPv6 addresses become `<address>`; anything shaped like an email address becomes `<email>`.
5. The text after `Audio device:` up to the first comma becomes `<device>`, as do names in control surface and input errors.
6. In stack lines, hex addresses and `+ offset` are removed and module paths are cut to the file name.
7. A run of 24 or more characters from the base64 or hex alphabets becomes `<key>`.
8. Text inside quotation marks in an error message becomes `<name>` unless it is one of the app's own fixed words (a command, effect or format name).
9. Lines are cut to 400 characters, and the log to its last 200 lines.

The rules err towards removing too much. The report window shows the result, so the user is the last check.

## Consent and controls

**Off by default, both kinds.** Nothing is sent, no ID exists and no counter is kept until a switch is turned on.

**Where the switches live:** a new **Privacy** section in the Settings window.

| Row | What it does |
|---|---|
| Send bug reports | Off. When on, a crash or an error box offers a report. Nothing is sent without the user pressing Send. |
| Send usage reports | Off. When on, counters are kept and one report a week is sent. |
| See what is sent | Opens the usage report as it would be sent now, as text, and the last reports sent (kept for 30 days in the app data folder). |
| This install's ID | Shows the ID, with Copy and Reset. Empty while both switches are off. |
| Delete what was sent | Asks the relay to file a deletion request for this ID. Turning both switches off offers the same with one tick box. |
| Privacy note | Opens the note in the guide's window. |

**How the user first hears of it** is a decision (see Decisions). Recommended: **no question at first run.** One plain line on the Home screen, once, from the third launch: "Torqwave can send optional bug and usage reports. They are off. Look in Settings > Privacy." with Open Settings and Dismiss. It is never shown again, whichever is pressed. A first-run question gets more yeses, and is also the first thing a new user meets before they have any reason to trust the app.

**Help > Report a Problem...** works whatever the switches say. It builds a report, shows it in full, and offers Copy, Save and, in a release build with a relay, Send. Each send is its own yes. A box for a note says that what is typed there is sent as typed.

**After a crash.** At the next start, if the log's last run ends in `Crashed.` and Send bug reports is on, one box: "Torqwave closed unexpectedly last time." with Look at the Report and Not Now. It comes after the recovery offer, never in front of it. With the switch off, nothing is shown; Report a Problem will still pick the crash up.

**Opting out.** Turning both switches off stops everything at once, deletes the ID, the counters and the kept copies of sent reports, and cancels a send in progress.

**What the README and the privacy note must say** (a short `docs/PRIVACY.md`, linked from the README and shown in the guide's window):

- that reports are optional and off, and that the app works the same without them;
- exactly what each kind holds, with the two examples above, and what is never sent;
- that the ID is random, where it is kept and how to reset or delete it;
- where reports go: a relay run by the developer on a named hosting company, then a private GitHub repository; that the hosting company and GitHub handle the data on the developer's behalf, and in which countries;
- that the relay does not store IP addresses, and that the hosting company necessarily sees one while handling a request;
- how long reports are kept: usage rows 13 months, bug reports 12 months after their issue is closed;
- how to have everything sent under an ID deleted (the button, or an email address or the public issue tracker with the ID), within 30 days;
- that builds made from the source by others send nothing unless whoever built them set a relay of their own;
- who is responsible (the developer, by the project's name and a contact address) and the date and version of the note. A change to what is collected raises the version, turns the switches off and asks again.

**Law, at the level a careful solo developer needs. This is not legal advice.**

- Under the GDPR and laws like it, a random ID tied to an install counts as personal data, and so does a typed note. The lawful basis here is **consent**: asked for separately for each kind, not ticked in advance, as easy to withdraw as to give, and recorded (the date and the note's version, kept locally).
- Storing the ID on the user's computer also falls under the EU's rules on storing things on a device; the same consent covers it, since the ID is only made after the yes.
- **Data minimisation:** collect what answers a question the developer actually has. Every field above should be able to name its question; remove any that can't.
- **Deletion and access on request** are possible because everything is filed under the ID. Without the ID the developer cannot find one person's reports, and should say so.
- **Retention** must be stated and kept (see the repository's layout below).
- **The hosting company and GitHub** act as processors, and both are based in the United States. Their standard data processing terms apply; the developer should read and accept them and name both in the note. Not checked here.
- **Children:** the app is not aimed at them, and reports hold nothing that says who a user is.
- If any of this is to ship to many users in the EU or the UK, an hour with someone qualified is worth its cost.

## The private repository

One private repository (called `torqwave-reports` here). The relay's token reaches this repository alone.

**Bug reports are issues.**

- Title: `[crash] c41e77a0 SessionMixer::render (0.20.0, Windows x86_64)`.
- Body: the report's fields as a short table, then the stack, the note and the log, each in a code block.
- Labels: `kind:crash`, `kind:error` or `kind:manual`; `os:macos`, `os:windows`, `os:linux`; `ver:0.20`; `new` until looked at.
- **Deduplication:** the relay keeps a table of signature to issue number in its store. A report whose signature is known becomes a one-line comment on that issue (version, system, the first 8 characters of the ID, the note if any) and adds the label `seen:many` at five. At most ten comments a day on one issue; the rest are counted, not written. If the table is lost, a duplicate issue is made and closed by hand.
- A `manual` report has no signature and is always a new issue.

**Usage reports are files** on a branch of their own, `data`:

- `usage/2026/2026-11-22.ndjson`: one line for each report received that day, written once by the relay's daily timer.

**A weekly rollup,** made by a scheduled workflow in the private repository (one short Linux job a week):

- `rollups/2026-W47.md`: installs reporting, by version, system and architecture; sessions and days used; the 30 most used commands and the commands nobody used; built-in instruments and effects by use; new crash signatures and the five most seen.
- The same text is posted as a comment on one standing issue, "Weekly rollup", so a notification arrives.

**Retention and deletion,** by the same workflow:

- usage files older than 13 months are removed, and the `data` branch is rewritten as a single commit so removed rows do not stay in its history;
- closed bug issues older than 12 months are deleted;
- an issue labelled `delete-request` (filed by the relay with an ID) is handled by a script that removes that ID's usage rows and lists its issues for deletion, then closes the request.

**From a report to the fix log.** A report in the private repository is not the public record. When a report shows a real problem:

1. The developer opens an issue in the public repository in their own words, with nothing copied from the report that the redaction should have caught.
2. The private issue gets the public issue's number in a comment and the label `public:#N`.
3. The fix commit adds the row to [DOGFOODING.md](DOGFOODING.md) as now, with "Found in" naming the version and "from a user's report" in the description, and closes the public issue. The private issue is closed with the release that carries the fix.

## Steps

### Step 1: a report built from the log, with private things taken out (M)

`Source/ProblemReport.h`: builds the bug report above from the app's version, the system, the audio device's numbers, the session's plug-ins and the log's last lines, and applies the backstop redaction rules. No network, no ID.

**Builds on:** the log file, `BuildStamp`, `juce::SystemStats`, the engine's device and known plug-ins.

**Tests** (no network): each redaction rule on lines made for it, in the three systems' path styles; a log written by a run of the tests with a made-up home folder, user name, host name, file names and device names, none of which appear in the report; a report never exceeds its caps; a report is valid JSON matching the schema; the same crash stack with different addresses gives the same signature, and a different top frame gives a different one.

### Step 2: Help > Report a Problem... (S)

A window that shows the report in full, a box for a note, the "Include plug-in names" tick box, and Copy, Save As and Open the Issue Page (the public repository's new-issue page; the report is on the clipboard, not in the address).

**Builds on:** the command table (a new command, described, performed and in the Help menu, as `Scripts/check_commands.py` requires), the guide's window for the text view.

**Tests:** the command passes the command table audit; unticking plug-in names removes every name from the text shown and copied; what is saved equals what is shown, byte for byte. By hand on each system: the window, Copy and Save.

**This step can ship alone. It is the zero-infrastructure version.**

### Step 3: private values marked where they are logged (M)

The logging helper and its markers; `showError`, `askThreeWay` and the direct `writeToLog` calls converted, a group of callers at a time. The log on disk reads as before.

**Tests:** a marked path, name and device are replaced in a report and present in the log; a check in the test suite (a script like `check_commands.py`) fails on a new `writeToLog` or `showError` call that passes a file's name or path unmarked.

### Step 4: the ID and the Privacy section (S)

`Source/Reports.h`: `Reports.xml`, the two switches, the ID's making, reset and deletion, the consent's date and version. The Privacy section in Settings with every row from the table above; the sending rows do nothing yet and say so. The one-time line on the Home screen, if chosen.

**Tests** (no network): no file and no ID exist before a switch is turned on; turning one on makes an ID of 32 hex characters; two made in a row differ; turning both off deletes the ID, the counters and the kept reports; Reset changes it; the ID never appears in the settings file, the log or a session file; a raised consent version turns both switches off.

### Step 5: crash signature and the offer at the next start (S)

Finds a crash at the end of the last run's log, builds the report with its signature, and offers it after the recovery offer when Send bug reports is on. An error box gets a small "Report..." button when the switch is on.

Also: each release's symbol files (dSYM, PDB, and the unstripped Linux binary) are kept by the release workflows, so a stack of offsets can be resolved. **(needs the developer:** where they are kept.)

**Tests:** a log ending in a made-up crash yields a report with the expected signature; a log ending normally yields none; the offer never appears with the switch off; it appears once for one crash, not at every start. By hand: a forced crash in a test build on each system, and what the stack looks like there.

### Step 6: the relay (M) **(needs the developer)**

`Relay/`: the function, its schema checks, its caps, the kill switch, the signature table, the daily usage file and the deletion request. Its tests run locally against a made-up GitHub API.

Needs, each a decision for the developer and done by them: an account with the hosting platform; the private repository; a fine-grained token limited to that repository with Issues: write and Contents: write, stored as the relay's secret; a host name; the labels and the two workflows in the private repository.

**Tests** (local, no accounts): each malformed request is refused; each cap holds and answers `429`; the kill switch answers `410`; a repeated signature becomes a comment, a new one an issue; the daily file holds each row once; `@` and formatting in a report are made harmless; no IP address appears in anything stored or sent to GitHub. Then, once deployed: one real report of each kind from a test build, found in the private repository, and the same with the token removed (the relay answers `503`, the app shows nothing).

### Step 7: Send (S)

`Source/ReportSender.h`: one POST of JSON on a background thread with a 10 second limit, sharing a small helper with `Ai::Client::open`. The Send button in the report window, the `410` and `429` handling, the kept copy of each sent report, and Delete what was sent.

**Tests** (against `MockServer`, no network): the body sent equals the report shown; `200`, `400`, `410`, `429`, `503`, no answer and a slow answer each leave the app as the design says; nothing is sent with the address unset; nothing is sent after opting out, including a send in progress; start-up and quit take no longer with the server unreachable.

### Step 8: usage counters and the weekly report (M)

Fixed names for commands (generated from the `cmd...` identifiers and checked by the command table audit, so a renamed command is caught), a counter in `perform`, counters where built-in instruments and effects are added, session and day counts, the weekly send, and See what is sent.

**Tests** (no network, then `MockServer`): no counter is kept with the switch off; every command has a fixed name and no two share one; a counted command shows in the report as its name, never its number; the report holds nothing but the schema's fields; one report a week at most, the counters cleared only after a `200`; a failed send is tried once more and then dropped; a third-party plug-in's name never appears.

### Step 9: the rollup, retention and the note (S) **(needs the developer)**

The two workflows in the private repository, `docs/PRIVACY.md`, the README's paragraph, a guide page, and the changelog entry that says what was not tried by hand.

**Tests:** the rollup script on a folder of made-up usage files gives the expected counts; the retention script removes rows by date and by ID and leaves the rest byte for byte; the note's version in the app matches the document's.

### Order

1. Steps 1 and 2 first. They can go into a release on their own.
2. Step 3 next, and it can go on in the background: each group of callers converted makes reports safer.
3. Step 4, then 5.
4. Step 6 only after the developer has agreed to the accounts. Step 7 after it.
5. Step 8, then 9. Nothing is sent to real users before step 9's privacy note is published.

## Decisions for the developer

1. **Build the relay at all, or stop after step 2?** Recommended: ship steps 1 to 3, use them for a release, then decide.
2. **Where the relay runs.** Recommended: a serverless function on a free plan with a timer and a small store.
3. **A fine-grained token or a GitHub App.** Recommended: the token first.
4. **The relay's address.** Recommended: a host name under a domain the project controls.
5. **How users first hear of it.** Recommended: Settings, plus one line on the Home screen once.
6. **Plug-in names in bug reports.** Recommended: yes, with the tick box, starting ticked for crashes.
7. **Usage reports at all.** Recommended: last, and only if steps 1 to 7 show that reports are being read and used.
8. **How long to keep things.** Recommended: usage 13 months, bug reports 12 months after closing.
9. **The contact for deletion requests** named in the privacy note.

## Risks, and what not to do

- **A private thing gets through the redaction.** This is the main risk. The answers are marking at the source, a backstop that removes too much, the user seeing every report before it goes, a private repository, and the rule that nothing from a report is copied to the public tracker.
- **The relay is abused.** The caps bound the cost: at worst a few hundred junk issues a day, deleted with one script, and the kill switch.
- **The relay is forgotten.** A token expires or a platform changes its terms, and reports stop. The app shows nothing, so the developer has to notice: the weekly rollup arriving is the sign that it works, and its absence the alarm.
- **The crash handler itself.** It builds a stack trace and writes a file from inside a crash, which is not always safe and can hang on some crashes. It is as it is today; this plan does not add to what runs there.
- **A promise broken later.** A field added quietly breaks the consent given. The schema has a version, the privacy note has a version, and a change to either asks again.
- **Trust.** For an open-source audio tool, the word "telemetry" costs goodwill even when it is off. Call them reports, keep them off, and keep the relay's source public.

**Do not:**

- put any token, key or password in the app or the repository;
- derive the ID from hardware, the network, the account or the computer's name, or add a second identifier that survives a reset;
- upload a crash, a log or anything else without the user having seen it and pressed Send (usage reports are the one automatic send, and only with their own switch on);
- send the log unredacted, the settings file, a session, the recovery copy or any audio;
- add a third-party analytics or crash SDK, or send anything to a service other than the relay;
- store or log IP addresses, or work out a country or city from one;
- ask at every start, ask again after a no, or make any feature depend on the switches;
- name third-party plug-ins in usage reports;
- send anything from test builds or from builds made by others;
- let the app wait for the relay, at start-up, at quit or anywhere in between.
