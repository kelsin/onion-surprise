# Surprise (Onion OS app)

C + SDL 1.2 app for Onion OS on the Miyoo Mini: Apps → Surprise lists the games in `/mnt/SDCARD/Roms/surprise.json` and launches them the way Onion's Random Game does. See README.md for scripts, `docs/surprise-json.md` for the input contract, and `docs/research/onion-internals.md` for Onion facts with source citations.

## Build and test

- `scripts/build.sh` cross-compiles in Docker (Colima, arm64 toolchain image) and writes `App/Surprise/surprise`.
- `make test` runs the host unit tests (no SDL) and the generator test. Run it before every commit.
- `scripts/preview.sh` renders headless screenshots to `build/preview/`. It uses the SDL dummy driver, so it can't show device-only rendering bugs.
- `scripts/package.sh` writes `dist/Surprise-<git describe>.zip`, which holds `App/Surprise/` and `Icons/Default/app/surprise.png`.
- Releases: release-please tags `vX.Y.Z` from conventional commits and CI attaches `Surprise-vX.Y.Z.zip`; `scripts/publish.sh` copies it to `/Volumes/roms/os/onion/`. Never put anything else there. See README "Releases".

## Device rules (verified on hardware)

- **Images:** never use `SDL_DisplayFormat*`, because Miyoo's libSDL draws those surfaces rotated 180°. Blit skins exactly as `IMG_Load` returns them, rotate only `background.png` 180°, and keep box art as software ARGB. Research §6 has the full results.
- **Exit code:** the app must always exit 0. Cancelling touches nothing. The launch contract is in research §2.
- **Values in surprise.json and recents lines** may not contain `"`, `:` or `\`, and each must be under 256 B, because Onion's parsers are naive.
- **`App/Surprise/config.json`:** keep it compact, one key per line, no space after `:`.
- **Icons:** Onion swaps app icons by basename (`surprise.png`) when a theme or icon pack is applied. Run `scripts/make-theme-icons.py <Themes dir>` to add matching icons to one-colour themes.

## Testing on the device

- The test device is a plain Miyoo Mini with no Wi-Fi, so `scripts/deploy.sh` (SSH) can't be used.
- Install by extracting the zip at the SD root, then run `dot_clean -m` on the folders you wrote to, to remove the macOS `._*` files.
- The app's log is `App/Surprise/log.txt`. Miyoo's libSDL writes its own debug lines there, so look for lines from this app.
- Checklist: `docs/device-testing.md`.

## Licensing

Our code is MIT. Onion and RomM are GPL-3.0: reimplement from the research notes and never copy their source.
