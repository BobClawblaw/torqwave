# Torqwave Project Constitution (`AGENTS.md`)

Rules and release standards that every agent and contributor must follow in this repository.

## 1. Git & Attribution Hygiene
- **Zero co-attribution:** Never add `Co-Authored-By:` trailers or any AI attribution in commits, annotated tags, GitHub release notes, documentation, or announcements.
- **Keep scratch/build artifacts untracked:** Never commit `tmp/`, `build*/`, or `dist/`.

## 2. Build, Verification & Local Install
- **Synth bit-identity:** Whenever modifying `WaveSynth`, run `TorqwaveTests --synth-identity` on an optimized build and confirm 100% sample-for-sample identity on all existing patches, presets, reference renders, and sessions.
- **Universal macOS verification:** Before tagging a release, run `bash Scripts/package.sh --jobs 8` and verify the full test suite passes on **both** `arm64` and `x86_64` (Rosetta).
- **Update `/Applications/Torqwave.app`:** After every macOS release build, install the newly built universal `Torqwave.app` to `/Applications/Torqwave.app` and verify `CFBundleShortVersionString` and `codesign --verify --deep --strict`.
- **Cross-platform CI:** Keep `TorqwaveTests --headless` passing on Windows (`windows-2025`) and Linux (`ubuntu-24.04` via `xvfb-run`).

## 3. Documentation, Comparison Matrix & Release Marketing Pipeline
Every release must ship complete, accurate documentation and a marketing kit tracked under **`docs/releases/<version>/`**:

1. **Docs & Site:** Update `CMakeLists.txt`, `Source/AiAssistant.cpp` (`releaseNotesFor`/`aboutThisRelease`), `CHANGELOG.md`, `README.md`, `ROADMAP.md`, `docs/_config.yml`, `docs/index.html`, `docs/quality/index.html`, `docs/TESTING.md`, `docs/AI.md`, and relevant `docs/guide/*.md` chapters.
2. **Comparison Snapshot:** Create `docs/compare/data/snapshots/<version>.json` (including `release_numbers`, `release_firsts`, `hero_tagline`, and `hero_cards`) and update `docs/compare/README.md`.
3. **Marketing Kit Pipeline:** Run `python3 Scripts/release_kit.py --version <version>` (or `Scripts/package.sh --release-kit`) to generate and verify:
   - Updated `docs/compare/MATRIX.md` and all 7 1600×900 SVGs + PNGs (`0-release-hero` through `6-the-whole-matrix`).
   - **`docs/releases/<version>/x/`** — numbered 1600×900 PNGs ready to drag into X.com.
   - **`docs/releases/<version>/instagram/`** — 7-slide 1080×1350 Instagram carousel (`slides.html` + `slide-1.png` through `slide-7.png`).
   - **`docs/releases/<version>/ANNOUNCEMENT.md`** — copy-paste-ready X.com long post, verified 280-char X thread (`generate.py --posts`), Instagram caption, and alt texts (`<= 1,000` chars).
   - **`docs/releases/<version>/manifest.json`** — tracks release metadata, matrix coverage, generated assets, and optional posting status for X.com and Instagram.
4. **Visual QA:** Always visually inspect the generated X and Instagram PNGs before committing to confirm clean typography, contrast, and zero clipped or overlapping text.
5. **Honesty Standard:** Never exaggerate coverage or hide limitations; state what Torqwave lacks (and what has not been tested by hand) as plainly as what it has.

## 4. Brand Identity Standard
- **Official Wordmark & Alternate Mark:** `"TORQ"` is always white (or strong ink on light themes) and `"WAVE"` + brand header accent bars are always Torqwave Hot Orange (`#ff5c1f` on dark surfaces, `#d9480f` on light surfaces — matching `Art/ridges.py` `HOT`).
- **Consistency Across Surfaces:** Apply this white + orange mark across all built-in plugins (`Theme::brandOrange()` in `WaveSynthEditor`, `SamplerEditor`, `SwarmGuitarEditor`), GitHub Pages (`docs/`), X.com graphics (`docs/compare/generate.py`), and Instagram carousels (`Scripts/release_kit.py`). Never use blue or teal/mint for the Torqwave wordmark or brand header bar.
