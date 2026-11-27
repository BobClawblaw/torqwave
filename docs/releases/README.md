# Release Marketing Kits (`docs/releases/`)

Every Torqwave release stores its marketing copy, comparison & social graphics, and optional announcement tracking in `docs/releases/<version>/`:

```text
docs/releases/<version>/
├── ANNOUNCEMENT.md      # Copy-paste-ready X.com long post, verified 280-char X thread, Instagram caption & alt texts
├── manifest.json        # Release date, matrix score, generated asset list, and optional X / Instagram posting status
├── x/                   # Numbered 1600×900 PNGs ready to drag into X.com (01–04 main post, 05–08 optional reply)
└── instagram/           # 7-slide 1080×1350 carousel (slides.html + slide-1.png..slide-7.png) ready for Instagram
```

## Generating a Release Kit

1. Add/update the comparison snapshot in `docs/compare/data/snapshots/<version>.json` (including `hero_tagline` and `hero_cards`).
2. Run the pipeline script:

```bash
python3 Scripts/release_kit.py --version <version> --open
```

Or include it directly during macOS release packaging:

```bash
bash Scripts/package.sh --jobs 8 --release-kit
```

3. Visually inspect the generated graphics in `docs/releases/<version>/x/` and `docs/releases/<version>/instagram/`.
4. Posting to X.com and Instagram is optional; when posted, record the post URL and `"posted": true` in `docs/releases/<version>/manifest.json`.
