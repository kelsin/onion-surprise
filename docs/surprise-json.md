# surprise.json contract

For `sync-roms`: write `/mnt/SDCARD/Roms/surprise.json`.

## Format

JSON Lines: one compact object per line (no spaces, no pretty-printing), same format as Onion's recents. Keys in this order:

`label, rompath, imgpath, launch, type`

- `label`: display name.
- `rompath`: `/mnt/SDCARD/Emu/<SYS>/../../Roms/<SYS>/<file>`
- `imgpath`: optional box art, `/mnt/SDCARD/Emu/<SYS>/../../Roms/<SYS>/Imgs/<stem>.png`
- `launch`: the emulator's launch script, `/mnt/SDCARD/Emu/<SYS>/launch.sh`. Resolve it from the `launch` key of the `Emu/<SYS>/config.json` you already target (it is relative to that Emu folder).
- `type`: always `5`.

Path strings always use `/mnt/SDCARD`, whatever the local mount point. Paths are written verbatim into the recents file, so they must not be normalised.

## Constraints

- No `"` and no `:` in any value (Onion parses recents with a naive scanner; strip them from labels).
- Every value under 256 bytes; every line under 1024 bytes.
- Box art is downscaled by the app to at most 250 px wide; pre-scaling to about 250x360 avoids work on the device.

## Example

```
{"label":"Golden Sun","rompath":"/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Golden Sun (USA).gba","imgpath":"/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Imgs/Golden Sun (USA).png","launch":"/mnt/SDCARD/Emu/GBA/launch.sh","type":5}
{"label":"EarthBound","rompath":"/mnt/SDCARD/Emu/SNES/../../Roms/SNES/EarthBound (USA).sfc","launch":"/mnt/SDCARD/Emu/SNES/launch.sh","type":5}
```

## How the app treats it

- Missing, empty or invalid file: shows "No surprise games yet" and B exits.
- Entries whose `rompath` or `launch` file does not exist on the card are skipped. A missing `imgpath` file just shows no art.
- Launching writes `/mnt/SDCARD/.tmp_update/cmd_to_run.sh`, prepends the selected line to Onion's recents (`Roms/recentlist.json` if `.tmp_update/config/.showRecents` exists, else `Roms/recentlist-hidden.json`), and exits so Onion runs the game.
