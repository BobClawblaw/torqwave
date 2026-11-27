---
activation: always_on
description: Torqwave project constitution covering git hygiene, local /Applications install, synth bit-identity, and the mandatory docs/releases/<version>/ marketing kit pipeline (X.com + Instagram).
---

# Torqwave Project Constitution

1. **Zero co-attribution:** Never add `Co-Authored-By:` or any AI attribution to git commits, tags, GitHub release notes, docs, or social announcements.
2. **Keep `tmp/`, `build*/`, and `dist/` untracked.**
3. **Rebuild, Deploy Locally & Refresh Release Packages:** Whenever app code changes, rebuild the universal macOS bundle (`bash Scripts/package.sh --jobs 8`), install to `/Applications/Torqwave.app` (verifying version, `x86_64 arm64`, and `codesign --verify --deep --strict`), and refresh the active GitHub Release `.dmg` + `.sha256` assets (`gh release upload v<version> --clobber ...`).
4. **Verify `--synth-identity`** after any `WaveSynth` change, and full universal `Scripts/package.sh` (`arm64` + `x86_64` Rosetta) before tagging a release.
5. **Release Marketing Kit (`docs/releases/<version>/`) is mandatory for every release:**
   - Update all docs (`CHANGELOG.md`, `README.md`, `ROADMAP.md`, `docs/index.html`, `docs/quality/index.html`, `docs/TESTING.md`, `docs/AI.md`, `docs/guide/*.md`) and `docs/compare/data/snapshots/<version>.json`.
   - Run `python3 Scripts/release_kit.py --version <version>` to regenerate `docs/compare/MATRIX.md` + graphics, stage **`docs/releases/<version>/x/`** (1600×900 X.com PNGs), render **`docs/releases/<version>/instagram/`** (7-slide 1080×1350 carousel PNGs + `slides.html`), write & validate **`docs/releases/<version>/ANNOUNCEMENT.md`** (`--posts`), and update **`docs/releases/<version>/manifest.json`** (tracking optional X.com & Instagram posting status).
   - Visually inspect generated PNGs before committing.
6. **Brand Identity Standard:** `"TORQ"` is always white/strong text and `"WAVE"` + brand header accent bars are always Torqwave Hot Orange (`#ff5c1f` on dark surfaces, `#d9480f` on light surfaces — matching `Art/ridges.py` `HOT`, via `Theme::brandOrange()` in built-in plugins). Never use blue or teal/mint for the Torqwave wordmark or brand chrome across plugins, GitHub Pages, or release graphics.
