# Surprise

A native C + SDL 1.2 app for [Onion OS](https://github.com/OnionUI/Onion) on the Miyoo Mini. Open it from Apps → Surprise to see a short list of games picked for you, with box art, and launch one the way Onion does.

The list comes from `/mnt/SDCARD/Roms/surprise.json`, written by the separate `sync-roms` tool. See [docs/surprise-json.md](docs/surprise-json.md).

![list view](docs/screenshot.png)

## Install

Extract `Surprise-<version>.zip` at the SD card root so `App/Surprise/` appears (it also installs `Icons/Default/app/surprise.png`), then open Apps → Surprise.

## Quick start (no sync-roms)

```sh
scripts/make-surprise-json.py /Volumes/<card> -n 10      # add --dry-run to preview
```

Picks random games that have `Imgs/<name>.png` box art and writes `Roms/surprise.json`.

## Build and test

```sh
scripts/build.sh          # cross-build in Docker (Colima) -> App/Surprise/surprise
make test                 # host unit tests + make-surprise-json test
scripts/preview.sh        # headless screenshots -> build/preview/*.png
scripts/package.sh        # build + dist/Surprise-<git describe>.zip (--no-build to skip the build)
scripts/publish.sh [--dry-run] [vX.Y.Z]   # copy a release's zip to /Volumes/roms/os/onion (default: latest release)
scripts/deploy.sh <host> [--run]   # copy to a device over SSH (tar over one connection); --run launches it and prints log.txt (press B only, MainUI is still running)
scripts/make-icon.py      # regenerate surprise.png (uv run --with pillow)
scripts/make-theme-icons.py <themes-dir> [--dry-run]   # one-colour Surprise icon per TechDweeb theme (uv run --with pillow)
```

On-device checklist: [docs/device-testing.md](docs/device-testing.md).

## Releases

Commits and PR titles follow [Conventional Commits](https://www.conventionalcommits.org/) (PR titles are checked in CI). Versions follow semver: `fix:` bumps the patch, `feat:` the minor, a breaking change (`feat!:` or a `BREAKING CHANGE:` footer) the major. The first release is v0.1.0.

1. On every push to `main`, [release-please](https://github.com/googleapis/release-please) keeps a release PR open with the next version and its [CHANGELOG.md](CHANGELOG.md) entry.
2. Merging that PR tags `vX.Y.Z`, creates the GitHub release, and CI cross-compiles and attaches `Surprise-vX.Y.Z.zip` (same layout as `scripts/package.sh`).
3. With the NAS share mounted, run `scripts/publish.sh` on this Mac to copy the zip to `/Volumes/roms/os/onion/`. It never overwrites an existing zip or deletes older ones; `--dry-run` checks everything without writing. `SURPRISE_STORAGE=<dir>` points it elsewhere.

Only tagged CI builds go to storage; local `package.sh` zips (named after `git describe`, possibly `-dirty`) are for testing.

## Licence

Our code is MIT ([LICENSE](LICENSE)). cJSON (`vendor/cjson`) is MIT. The bundled fallback font is under the licence in `App/Surprise/res/font-LICENSE.txt`.
