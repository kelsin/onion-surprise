# Onion OS internals for the "Surprise" app

Research date: 2026-11-18. All claims come from source; anything not confirmed is marked **UNVERIFIED**.

Sources (cloned read-only, nothing built or run):

| Repo | Ref | Notes |
|---|---|---|
| `OnionUI/Onion` | `main` @ `07505ea` (2026-01-21, VERSION `4.4.0-beta-20260120`, `Makefile:4`) | primary source |
| `OnionUI/Onion` | latest release tag `v4.3.1-1` @ `7dfc008` (2024-06-24) | re-checked the key launch, recents and theme code. Same behavior, only small line-number shifts |
| `rommapp/miyoo-mini-app` | `ec7ca58` (2025-01-05) | third-party SDL1.2 app |
| `shauninman/union-miyoomini-toolchain` | `cfcae30` (2025-11-11) | toolchain Dockerfile |
| `OnionUI/Themes` release zip "System 9 by injekim" | main | real theme layout |
| `shauninman/miyoomini-toolchain-buildroot-aarch64` v0.0.3 tarball | file listing only | sysroot contents |

Paths below are relative to the repo root (`Onion/…` unless noted). MainUI (`/mnt/SDCARD/miyoo/app/MainUI`) is closed-source, so its own behavior is inferred from the Onion code that talks to it.

**Key find:** Onion already ships an app that does almost exactly what we want: **"Random Game"** (`static/packages/App/Random Game/App/RandomGamePicker/{launch.sh,random.sh}` plus `src/randomGamePicker/randomGamePicker.c`). It picks a game, writes `cmd_to_run.sh`, prepends a recents line, touches `/tmp/quick_switch` and exits. Copy its contract.

---

## 1. App folder schema

Location: `/mnt/SDCARD/App/<DirName>/`. Onion lists an app if `/mnt/SDCARD/App/<dir>/config.json` exists (`src/common/utils/apps.h:19-38`).

`config.json` (all examples: `static/build/App/PackageManager/config.json:1-6`, `static/packages/App/*/App/*/config.json`):

```json
{
"label":"Surprise",
"icon":"icon.png",
"launch":"launch.sh",
"description":"Games picked for you"
}
```

- Keys used: `label`, `icon`, `launch`, `description`. `iconsel` (the selected-state icon) is optional; RomM sets `"iconsel": ""` (`miyoo-mini-app/App/RomM/config.json:4`).
- `icon` is resolved relative to the app dir. Onion uses `"../../Icons/Default/app/<name>.png"`, RomM uses `"romm.png"`.
- Icon format: PNG RGBA **74×74** (all of `static/build/Icons/Default/app/*.png`, checked with `file`). RomM ships 64×64, which also works.
- Icon-pack caveat: `themeSwitcher` / `apply_singleIcon` rewrites `icon`/`iconsel` in App config.json files when the icon pack changes. It does this only if the pack holds a file named `<icon basename before the first '-'>.png` (`src/common/utils/apply_icons.h:131-183`). Use a unique basename (e.g. `surprise.png`) so the icon is never replaced.
- Onion's `config.json` parser for `label`/`launch` is `file_parseKeyValue(config_path, "label", …, ':', 0)` (`apps.h:67,127`), a simple key:value scan. Keep one key per line, no nesting.

### How an app is launched (cwd/env)

When an app is picked in MainUI, MainUI writes `/tmp/cmd_to_run.sh` and exits. `runtime.sh` moves it into place and runs it. Onion's own equivalent (used by keymon shortcuts) shows the exact command format:

```c
// src/common/utils/apps.h:134-138
snprintf(cmd, STR_MAX * 4 - 1,
         "cd %s; chmod a+x ./%s; "
         "LD_PRELOAD=/mnt/SDCARD/miyoo/app/../lib/libpadsp.so ./%s",
         app_dir, launch, launch);
file_put_sync(fp, "/tmp/cmd_to_run.sh", "%s", cmd);
```

That MainUI writes byte-for-byte the same string is **UNVERIFIED** (binary). The `cd <appdir>;` form is confirmed by `runtime.sh:590-593`, which parses field 2 of the command as the app dir.

`static/build/.tmp_update/runtime.sh`:
- `:4-5`: global env inherited by everything launched: `LD_LIBRARY_PATH="/lib:/config/lib:/mnt/SDCARD/miyoo/lib:/mnt/SDCARD/.tmp_update/lib:/mnt/SDCARD/.tmp_update/lib/parasyte"` and `PATH="/mnt/SDCARD/.tmp_update/bin:$PATH"`. This puts `infoPanel`, `playActivity` and the other Onion tools on PATH.
- `:88`: `HOME=/mnt/SDCARD/RetroArch/`.
- `:226-239`: MainUI runs, then `mv -f /tmp/cmd_to_run.sh $sysdir/cmd_to_run.sh`.
- `:303-445` `launch_game()`: `cd /mnt/SDCARD/RetroArch` (`:413`), then `TZ="$TZ_VALUE" $sysdir/cmd_to_run.sh` (`:417`). The app's own `cd` sets cwd to the app dir.
- `is_game` is set only when the command contains `retroarch/cores`, `/../../Roms/` or `/mnt/SDCARD/Roms/` (`:278-280`). An app command is "not a game", so it gets no playActivity tracking.
- Exit code: retval ≥128 except 143/255 shows a "Fatal error occurred" infoPanel (`:440-441`). **The app must exit 0.**
- 560p: an app only runs at 752×560 if `/mnt/SDCARD/App/<dir>/full_resolution` exists and `/tmp/new_res_available` is set (`:401-404`, `:586-594`, `.github/create_fullres_files.sh:3-4`). Without that file the app runs at 640×480.

`launch.sh` conventions: `#!/bin/sh`, `cd` to the app dir (`miyoo-mini-app/App/RomM/launch.sh:2`, `Music Player (GMU)/App/Gmu/launch.sh:2-3`), and add local libs if needed (`LD_LIBRARY_PATH=…:./lib:$LD_LIBRARY_PATH`, GMU `launch.sh:11`).

---

## 2. Launching a game the way Onion does

### The mechanism (runtime main loop)

```sh
# runtime.sh:149-154
while true; do
    state_change check_main_ui     # only runs MainUI if $sysdir/cmd_to_run.sh is absent (:187-198)
    state_change check_game_menu
    state_change check_game        # runs $sysdir/cmd_to_run.sh if present (:271-276)
    state_change check_switcher
done
```

```sh
# runtime.sh:622-635
check_switcher() {
    if [ -f $sysdir/.runGameSwitcher ]; then
        launch_switcher
    elif [ -f /tmp/quick_switch ]; then
        rm -f /tmp/quick_switch          # keep cmd_to_run.sh → next loop launches it
    else
        rm $sysdir/cmd_to_run.sh 2> /dev/null   # back to MainUI
        sync
    fi
```

`launch_game` clears the flag *before* running the command (`rm -f /tmp/quick_switch`, `:385`). Anything the app touches while it runs therefore survives until `check_switcher`.

### Sequence for our app (same as Random Game)

1. MainUI exits. Runtime runs `cd /mnt/SDCARD/App/Surprise; … ./launch.sh` (is_game=0).
2. The app overwrites **`/mnt/SDCARD/.tmp_update/cmd_to_run.sh`** with the game command.
3. The app prepends a recents line (see §3).
4. The app (or launch.sh) runs `touch /tmp/quick_switch`, then exits 0.
5. `check_switcher` sees quick_switch and keeps the cmd. On the next loop `check_main_ui` is skipped and `check_game` launches the game with is_game=1. That path calls `playActivity start "$rompath"` (`:381`), applies the per-game core override from `<romdir>/.game_config/<stem>.cfg` (`:347,360-364`), resolves realpath (`:334-344`), and on exit runs `playActivity stop` (`:557`).
6. If the user cancels (B), the app exits 0 **without** touching quick_switch. `check_switcher` then deletes cmd_to_run.sh and MainUI comes back (`random.sh:29-33` also `rm -f $sysdir/cmd_to_run.sh`).

**No need to kill MainUI.** It has already exited before an app runs. Keymon kills MainUI only for shortcuts fired from inside MainUI (`src/keymon/menuButtonAction.h:25-30`).

### Exact command line format (identical in 4 Onion writers)

```c
// src/randomGamePicker/randomGamePicker.c:426-433
snprintf(cmd_to_run, STR_MAX * 3 + 64,
    "LD_PRELOAD=/mnt/SDCARD/miyoo/app/../lib/libpadsp.so \"%s\" \"%s\"",
    chosen_game->launch_path, chosen_game->path);
file_put_sync(fp, "/mnt/SDCARD/.tmp_update/cmd_to_run.sh", "%s", cmd_to_run);
```

The same format appears in `src/common/system/state.h:411,425` (GameSwitcher `resumeGame`), `src/gameSwitcher/gs_history.h:225`, and `static/build/.tmp_update/bin/adv/advexec.sh:11-12` (which also does `touch /tmp/quick_switch`). `file_put_sync` is fopen "w+", fprintf, fflush, fsync, fclose (`src/common/utils/file.h:46-54`). It writes no trailing newline.

- `launch` = absolute path to `Emu/<SYS>/launch.sh` (e.g. `/mnt/SDCARD/Emu/GBA/launch.sh`).
- `rompath` = absolute ROM path. The rom path **must be the last double-quoted token**: runtime extracts it with `index($0,"\" \"")` (`:321`) and playActivity takes the text after the last `"` (`src/playActivity/playActivityDB.h:411-429`).
- Paths containing `:` are treated as `launch_script:rompath` (`runtime.sh:324-328`, `state.h:372-388`, `gs_history.h:57-71`). **Reject ROM paths that contain `:`.** `$` is escaped by runtime (`:367-370`).

Launch writer in C:

```c
FILE *fp = fopen("/mnt/SDCARD/.tmp_update/cmd_to_run.sh", "w+");
fprintf(fp, "LD_PRELOAD=/mnt/SDCARD/miyoo/app/../lib/libpadsp.so \"%s\" \"%s\"", launch, rompath);
fflush(fp); fsync(fileno(fp)); fclose(fp);
fclose(fopen("/tmp/quick_switch", "w"));   // = touch
sync();
return 0;
```

### Other files

- **Recents**: nobody else writes them on this path, because MainUI writes recents only for games it launches itself. Onion's Random Game prepends manually:

```sh
# Random Game/App/RandomGamePicker/random.sh:36-52
if [ ! -f $sysdir/config/.showRecents ]; then currentrecentlist=$recentlist_hidden   # /mnt/SDCARD/Roms/recentlist-hidden.json
else currentrecentlist=$recentlist; fi                                               # /mnt/SDCARD/Roms/recentlist.json
recentFileLine="{\"label\":\"$label\",\"rompath\":\"$rompath\",\"imgpath\":\"$imgpath\",\"launch\":\"$launchpath\",\"type\":5}"
echo "$recentFileLine" | cat - $currentrecentlist > $recentlist_temp && mv $recentlist_temp $currentrecentlist
```

  (C code instead uses `getMiyooRecentFilePath()`: the hidden file if it exists, else the normal one, `state.h:228-238`. Both give the same answer at runtime because `check_hide_recents` runs around MainUI, `runtime.sh:668-686`.)

- **GameSwitcher** reads only the recents file (`gs_history.h:95-146`). It shows type 5/17 entries whose `rompath` and `launch` both exist, and deletes later lines whose `rompath` string equals an earlier one (`:118-131`). So prepend our line, and use the *same rompath string form MainUI uses* (see §3), or the game shows up twice.
- **playActivity**: automatic (runtime `:381`, `:557`). Nothing to write.
- `history` / RetroArch `content_history.lpl` (`settings.h:16-17`) is written by RetroArch itself. Nothing to write.
- Optional: `infoPanel --title … --message … --auto` for errors (`random.sh:8`).

---

## 3. `favourite.json` / `recentlist.json` line format

Files: `/mnt/SDCARD/Roms/favourite.json` (`src/common/components/JsonGameEntry.h:8`) and `/mnt/SDCARD/Roms/recentlist.json` / `recentlist-hidden.json` (`src/common/system/settings.h:18-19`). Format is **JSON Lines**: one object per line, newest first.

Fields read: `label`, `launch`, `type`, `rompath`, `imgpath` (`JsonGameEntry.h:10-40`, `gs_history.h:31-54`). The `emupath` field in the C struct is derived from `rompath` by splitting at `/../../` (`JsonGameEntry.h:36-37`), so it is not stored. Writer order (`JsonGameEntry.h:42-52`):

```
{"label":"…","launch":"…","type":5,"imgpath":"…","rompath":"…"}
```

`type`: 5 = game, 17 = expert-mode game, 3 = app (`randomGamePicker.c:194-199`). GameSwitcher and resume accept only 5 and 17 (`gs_history.h:32`, `state.h:262,345`).

Canonical path form (Onion's own comment, `src/common/utils/file.h:132`): `"/mnt/SDCARD/Emu/GBA/../../Roms/GBA/game.gba"`. Onion builds paths as `emupath + "/" + <relative config path>` (`randomGamePicker.c:73,80,94,285`), so entries look like:

```
{"label":"Metroid Fusion","rompath":"/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Metroid Fusion.gba","imgpath":"/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Imgs/Metroid Fusion.png","launch":"/mnt/SDCARD/Emu/GBA/launch.sh","type":5}
```

(That MainUI itself writes exactly this form is **UNVERIFIED**, since MainUI is closed source. Test on device by launching one game from MainUI and reading `recentlist.json`. The `/../../Roms/` form is what `JsonGameEntry`, `playActivityDB.h:237-238` and `cacheDB.h:110-111` all expect.)

### Parser constraints (hard requirements)

`resumeGame` uses `strstr` on the raw line, not a JSON parser (`state.h:343-397`):
- **compact JSON, no spaces** (`"rompath":"` is matched literally);
- **no escaped `"` inside values**, because values end at the first `"` (`strchr(…,'"')`). Strip or replace `"` in labels;
- `label`, `rompath`, `imgpath`, `launch` must each be **< 256 bytes**. `char label[256]` and friends are copied unbounded with `strncpy(…, end-start)` (`state.h:337-361`), which is a stack overflow risk;
- whole line < `STR_MAX*4` = 1024 bytes (`state.h:332`, `STR_MAX`=256 `src/common/utils/str.h:9`);
- `\n` line terminator, UTF-8.

### Proposed `/mnt/SDCARD/Roms/surprise.json`

Use the same format, which only our app reads, so we can copy a line from it into recents unchanged:

```
{"label":"<display name>","rompath":"/mnt/SDCARD/Emu/<SYS>/../../Roms/<SYS>/<file>","imgpath":"/mnt/SDCARD/Emu/<SYS>/../../Roms/<SYS>/Imgs/<stem>.png","launch":"/mnt/SDCARD/Emu/<SYS>/launch.sh","type":5}
```

- Same key order as random.sh. `imgpath` is optional: omit it when no box art exists (random.sh omits it when the file is missing, `:46-50`).
- The sync tool must apply every constraint above (compact, no `"`/`:` in values, field < 256 B).
- Recommendation: keep `surprise.json` strictly recents-compatible (no extra keys), so the app can prepend the selected line to recents verbatim. Unknown keys would probably be harmless to the strstr parser, but that is untested. If metadata is needed (e.g. a "why" blurb), put it in a sidecar file rather than in these lines.
- Do **not** name the file `*.json` inside an Emu `rompath` dir, or MainUI may list it as a ROM. `/mnt/SDCARD/Roms/` root is fine: favourite.json and recentlist.json already live there.

---

## 4. Emulator resolution and box art

`Emu/<SYS>/config.json` (e.g. `static/packages/Emu/Nintendo - Game Boy Color (Gambatte)/Emu/GBC/config.json:1-13`):

```json
{"label":"GBC","icon":"../../Icons/Default/gbc.png","iconsel":"…","launch":"launch.sh",
 "rompath":"../../Roms/GBC","imgpath":"../../Roms/GBC/Imgs","gamelist":"../../Roms/GBC/miyoogamelist.xml",
 "useswap":0,"shortname":0,"hidebios":0,"extlist":"bin|dmg|gb|gbc|zip|7z"}
```

All 51 packaged Emu configs have `label, icon, launch, rompath, imgpath, extlist, shortname, hidebios, useswap`, and almost all have `gamelist` and `iconsel`. Paths are **relative to the Emu dir**. Some configs have spaces around `:` (ScummVM, `…/Emu/SCUMMVM/config.json`), so use a real JSON parser.

Onion's resolver (`randomGamePicker.c:49-100`): `launch_abs = emupath + "/" + launch`, `romsdir = emupath + "/" + rompath`, `imgsdir = emupath + "/" + imgpath`. Box art is `imgsdir + "/" + <rom basename without extension> + ".png"` (`:283-288`). playActivity uses the resolved form `/mnt/SDCARD/Roms/<folder>/Imgs/<stem>.png` (`playActivityDB.h:51-62`). Rom dirs that point to `../../App/` are Search results and are skipped (`:67-71`).

Emulators may also live in `/mnt/SDCARD/RApp/<x>/` (`randomGamePicker.c:20,272-273`), and a rompath can belong to several emus (e.g. `Emu/GBA` vs `RApp/mgba`). The sync tool should record the exact `launch` it wants.

Recommendation: let the sync tool resolve everything and write absolute `launch` / `rompath` / `imgpath`. On-device the app only checks that `rompath` and `launch` exist (as GameSwitcher does, `gs_history.h:133`).

---

## 5. On-device libraries and the recommended stack

Shipped by Onion (SD card, from `static/build/`):

| Path | Contents |
|---|---|
| `/mnt/SDCARD/miyoo/lib/` | `libSDL-1.2.so.0` (Miyoo build), `libSDL_ttf-2.0.so.0`, `libSDL_image-1.2.so.0`, `libSDL_mixer-1.2.so.0`, `libpng12.so.0`, `libpng.so.3`, `libfreetype.so.6`, `libz.so.1`, `libpadsp.so`, `libshmvar.so`, … |
| `/mnt/SDCARD/.tmp_update/lib/` | `libSDL_gfx.so.15`, curl, ssl, crypto, poppler, … |
| `/mnt/SDCARD/.tmp_update/lib/parasyte/` | a separate generic set: `libSDL-1.2.so.0`, **`libSDL2-2.0.so.0`**, `libpng16.so.16`, `libjpeg.so.9`, glibc copies, … (later in LD_LIBRARY_PATH order, so the Miyoo SDL in `miyoo/lib` wins) |
| `/customer/lib`, `/config/lib`, `/lib` | stock firmware. Not in the repo. Contents **UNVERIFIED** |

- No JSON library ships on device. Onion compiles vendored cJSON (`include/cjson/cJSON.{c,h}`, MIT) into each binary (`src/common/config.mk:21-23`, `INCLUDE_CJSON=1`). RomM links `-ljson-c` (`miyoo-mini-app/Makefile:4`), which is **not** on device. Don't copy that.
- `libSDL_rotozoom.so` (which Onion tools link for `zoomSurface`) is built from `include/SDL/SDL_rotozoom.c` into repo `lib/` (`include/SDL/Makefile:3-8`). It is copied only into the installer dir (`Makefile:73`). Its installed runtime location is **UNVERIFIED**, so don't rely on it: compile `SDL_rotozoom.c` (SDL_gfx, zlib licence) into the app, or have the sync tool pre-scale art.
- SDL2: only the generic parasyte build, with no MI_GFX/MI_SYS backend, and the toolchain sysroot has no SDL2 headers. **Avoid it.**

What Onion's own C apps link (e.g. `src/gameSwitcher/Makefile:9`): `-lSDL -lSDL_ttf -lSDL_image -lSDL_rotozoom -lpng -lsqlite3 -pthread`. CFLAGS (`src/common/config.mk:34,53-54`):
`-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7ve -Wl,-rpath=/mnt/SDCARD/.tmp_update/lib -DPLATFORM_MIYOOMINI`, compiled `-std=gnu18` (`src/common/commands.mk:12`).

Toolchain sysroot (aarch64 v0.0.3 tarball listing, `arm-linux-gnueabihf/libc/usr/…`) has: `include/SDL/SDL.h`, `SDL_ttf.h`, `SDL_image.h`, `libSDL-1.2`, `libSDL_ttf-2.0`, `libSDL_image-1.2`, `libSDL_mixer`, `libpng12/16`, `libfreetype`, `libcjson`, `libmi_{sys,gfx,disp,ao,…}`. Toolchain is GCC 8.3.0.

**Recommended lowest-risk stack:** C + **SDL 1.2 + SDL_ttf + SDL_image** (dynamic, resolved from `/mnt/SDCARD/miyoo/lib` through the inherited LD_LIBRARY_PATH), vendored **cJSON** compiled in, and (optionally) `SDL_rotozoom.c` compiled in. Nothing else to bundle. Render with the classic path Onion's `tweaks` / `packageManager` use:

```c
// src/common/utils/sdl_init.h:17-39
SDL_Init(SDL_INIT_VIDEO); SDL_ShowCursor(SDL_DISABLE); SDL_EnableKeyRepeat(300, 50); TTF_Init();
video  = SDL_SetVideoMode(w, h, 32, SDL_HWSURFACE);
screen = SDL_CreateRGBSurface(SDL_HWSURFACE, w, h, 32, 0, 0, 0, 0);
/* draw into screen, then: */ SDL_BlitSurface(screen, NULL, video, NULL); SDL_Flip(video);
```

(`src/packageManager/render.h:176-177`, `src/tweaks/tweaks.c:78,246-249`)

---

## 6. Theme integration

- Current theme dir: `"theme"` key in **`/mnt/SDCARD/system.json`** (not `Saves/CurrentProfile/...`), e.g. `"/mnt/SDCARD/Themes/Silky by DiMo/"`. If the value is `"./"` or not a directory, fall back to `/mnt/SDCARD/miyoo/app/` (`src/common/theme/load.h:12,17,117-128`; runtime uses the same key via `/customer/app/jsonval theme`, `runtime.sh:38`).
- Theme `config.json` at `<theme>/config.json`, falling back to `/mnt/SDCARD/miyoo/app/config.json` (`theme/config.h:219-225`). User overrides are merged from `/mnt/SDCARD/Saves/CurrentProfile/theme/config.json` (`load.h:16`, `config.h:227-229`). Keys (`config.h:95-182`): `name, author, description, hideLabels{icons,hints}` (or legacy `hideIconTitle`), `title|hint|list{font,size,color}`, `currentpage|total{color}`, `grid{font,grid1x4,grid3x4,color,selectedcolor}`, `batteryPercentage{visible,font,size,color,offsetX,offsetY,textAlign|onleft,fixed}`, `frame{border-left,border-right}`. Colors are `"#RRGGBB"`.
- Fallback rules (`config.h:134-138,184-210`): hint falls back to title, and list falls back to title. Defaults: title 36, hint 40, list 24, white. Fonts are relative to the theme dir unless absolute. The final font fallback is `/customer/app/Exo-2-Bold-Italic.ttf` (`load.h:13,105-115`). The list font is rendered **bold** (`resources.h:256-259`). Default theme list font: `/mnt/SDCARD/miyoo/app/wqy-microhei.ttc` size 25 (`static/build/miyoo/app/config.json:34-38`).
- Image lookup (`load.h:41-71`): `Saves/CurrentProfile/theme/skin/<name>.png`, then `<theme>/skin/<name>.png`, then `/mnt/SDCARD/miyoo/app/skin/<name>.png` (the `extra/<x>` names fall back to `/mnt/SDCARD/.tmp_update/res/<x>.png`).
- Skin names a game list needs (`resources.h:139-191`): `background` (640×480), `bg-title` (header 640×60), `bg-list-s` (row highlight 640×60), `div-line-h` (640×4), `tips-bar-bg` (footer 640×60), `preview-bg` (box-art panel 256×364), `icon-A-54`, `icon-B-54`, and `power-*-icon` (battery). Sizes are from the default skin `static/build/miyoo/app/skin/*.png`.
- Onion's MainUI-style list layout (`src/common/theme/render/list.h:52-235`, header `render/header.h:9-55`, footer `render/footer.h:63-125`): header y 0–60 with a centered title (TITLE font). Rows start at y=60: **6 small rows × 60 px** (`components/list.h:117`), with `div-line-h` dividers and a `bg-list-s` highlight on the active row. The label is drawn at x=20 with the LIST font and vertically centered. Box art: `preview-bg` is blitted at (640−w, 60), and the image is downscaled to ≤250 px wide and centered at x within the panel, y=240 (`list.h:186-217`, `preview_width = 250`). Footer y 420–480 shows `tips-bar-bg`, A/B hints and "n/total" in the HINT font at right x=620, y=449 (`footer.h:73-110`). How closely this matches MainUI's own *game* list is **UNVERIFIED**, since MainUI is closed source.
- **Rotation quirk (UNVERIFIED reason):** `theme/background.h:12-18` loads `skin/background.png` and **rotates it 180°** (`utils/rotate180.h`) before use, in both main and v4.3.1-1. Yet a real theme's `background.png` is stored upright (checked "System 9": title bar at the top). Don't reuse `background.h`. Blit `background.png` as-is with the classic SDL path, and check orientation on device in the first smoke test.
- Reuse and licensing: Onion is **GPL-3.0** (`LICENSE`), and so is RomM's app. Vendoring `theme/*.h`, `components/list.h` or `utils/*` makes the app a GPL-3.0 derivative. Those headers also drag in many dependencies (`system/lang.h`, `utils/flags.h` temp flags, keymon shm, `SDL_rotozoom`) and keep all state in header-defined statics. Recommendation: reimplement a small theme loader (~150 LOC) on cJSON (MIT) and the SDL_gfx rotozoom code (zlib), following the paths and keys above. Or license the app GPL-3.0 and vendor `load.h` + `config.h` + `render/list.h` with edits.

---

## 7. Input mapping (SDL 1.2 keysyms)

```c
// src/common/system/keymap_sw.h:7-22   (raw evdev codes in keymap_hw.h:7-24)
UP SDLK_UP  DOWN SDLK_DOWN  LEFT SDLK_LEFT  RIGHT SDLK_RIGHT
A  SDLK_SPACE      B  SDLK_LCTRL     X SDLK_LSHIFT   Y SDLK_LALT
L1 SDLK_e          R1 SDLK_t         L2 SDLK_TAB     R2 SDLK_BACKSPACE
SELECT SDLK_RCTRL  START SDLK_RETURN MENU SDLK_ESCAPE POWER SDLK_FIRST
// evdev: VOL+ KEY_VOLUMEUP, VOL- KEY_VOLUMEDOWN, POWER KEY_POWER, input device /dev/input/event0 (sdl_direct_fb.h:22)
```

Use A = launch, B = exit (Onion hint convention `LANG_SELECT`/`LANG_BACK`, `tweaks.c:239`). keymon keeps running during apps (`runtime.sh:159`). In app state (`MODE_APPS`) the power button triggers `remove(CMD_TO_RUN_PATH); suspend(1)` (`src/keymon/keymon.c:242-244`). Handle SIGTERM/SIGINT cleanly, as `tweaks.c:74-75` does.

---

## 8. Screen sizes and rotation

- Models: `283` = Mini (v1–v3, v4), `354` = Mini Plus, detected by whether an `axp` PMIC is present (`runtime.sh:10-18`). The result is written to `/tmp/deviceModel` (`src/common/system/device_model.h:7-23`). **Mini Flip has no model-specific code** in Onion main (grep found none), so Flip support is **UNVERIFIED**.
- Physical panel: read from `/proc/mi_modules/fb/mi_fb0` and written to `/tmp/screen_resolution`. 752×560 is used only with firmware ≥ `202310271401`, which also sets `/tmp/new_res_available` (`runtime.sh:713-742`).
- The **framebuffer stays 640×480** for MainUI and for apps. It switches to 752×560 (`fbset -g 752 560 752 1120 32`) only for programs with a `full_resolution` marker, and switches back afterwards (`runtime.sh:282-301,400-423`).
- Onion C code always takes its render size from the fb (`display_getRenderResolution()` → `FBIOGET_VSCREENINFO`, `display.h:83-92`). When the fb isn't 640×480 it scales theme assets by `width/640` (`sdl_direct_fb.h:37-39`, `load.h:24-39`).
- **Recommendation:** ship without `full_resolution`. Then the app always runs at 640×480, theme assets match 1:1, and there is nothing to handle for 560p. Still size the surface from `SDL_SetVideoMode(0,0,…)` or the fb vinfo rather than hard-coding.
- Rotation: the panel is mounted rotated. Onion's **direct framebuffer** renderers (gameSwitcher, infoPanel, bootScreen) rotate 180° when writing to `/dev/fb0` (`sdl_direct_fb.h:63-70`, `display_writeBuffer(..., rotate=true, ...)`; screenshots likewise `screenshot.h:91-119`). The **classic SDL path** (SDL_SetVideoMode + SDL_Flip on Miyoo's libSDL) does no manual rotation in Onion code (`tweaks`, `packageManager`, `themeSwitcher`), so the Miyoo SDL build handles it. That the handling happens inside Miyoo's libSDL is an inference, not checked in source. → **Use the classic SDL path and never write `/dev/fb0` directly.** See §6 for the open question about the background image.

---

## 9. Toolchain

- Onion builds with **`aemiii91/miyoomini-toolchain:latest`** (`Makefile:51,255-264`; CI `.github/workflows/build.yml:12-24`: `source /root/.bashrc; make build`). Docker Hub (queried 2026-11-18) lists only **amd64** for that tag, so there is no arm64 image.
- `shauninman/union-miyoomini-toolchain` is the base design. `Dockerfile` = `debian/eol:buster-slim` (multi-arch, arm64 included) plus `support/setup-toolchain.sh`, which picks the **aarch64-host** toolchain when `uname -m` is `aarch64` (`setup-toolchain.sh:6-13`). Both release blobs v0.0.3 exist (HTTP 200). `setup-env.sh:1-4` sets `CROSS_COMPILE=/opt/miyoomini-toolchain/usr/bin/arm-linux-gnueabihf-`, `PREFIX=/opt/miyoomini-toolchain/usr/arm-linux-gnueabihf/sysroot/usr` and `UNION_PLATFORM=miyoomini`. The aarch64 tarball includes the `usr` and `arm-linux-gnueabihf/sysroot` symlinks, so those paths resolve (from the tarball listing; that the symlinks work was not tested).

### Apple Silicon + Colima

Option A (recommended): native arm64 container, no emulation.

```sh
brew install colima docker            # both already present on this Mac
colima start --cpu 4 --memory 8 --disk 40
git clone https://github.com/shauninman/union-miyoomini-toolchain ~/src/union-miyoomini-toolchain
docker build -t miyoomini-toolchain ~/src/union-miyoomini-toolchain     # downloads ~278 MB aarch64 toolchain
docker run --rm -v ~/src/onion-surprise:/root/workspace miyoomini-toolchain \
  /bin/bash -c 'source /root/.bashrc && make'        # Makefile uses $(CROSS_COMPILE)gcc, -I$(PREFIX)/include -L$(PREFIX)/lib
```

Option B: Onion's exact image (amd64, emulated).

```sh
colima start --vm-type vz --vz-rosetta --cpu 4 --memory 8     # or: colima start --arch x86_64 (qemu, slower)
docker run --rm --platform linux/amd64 -v ~/src/onion-surprise:/root/workspace \
  aemiii91/miyoomini-toolchain:latest /bin/bash -c 'source /root/.bashrc && make'
```

Both commands were written from the sources and are **not run**. Whether the aemiii91 image's sysroot matches the union aarch64 sysroot is **UNVERIFIED**. Option A's sysroot was confirmed to contain SDL 1.2 / ttf / image / png / cjson.

- **Non-Docker path:** the toolchain binaries are Linux ELF (x86_64 or aarch64 host), so they don't run on macOS. Alternatives: (a) `colima ssh` and build inside the Linux VM after extracting the tarball (still a VM, but no Docker); (b) macOS `clang --target=arm-linux-gnueabihf -march=armv7ve -mfloat-abi=hard --sysroot=<extracted libc> -fuse-ld=lld` against the extracted sysroot. Option (b) is **UNVERIFIED** and fragile (crt files and GCC lib paths). Use Docker Option A.

### Makefile sketch

```make
CC      ?= $(CROSS_COMPILE)gcc
CFLAGS  = -O2 -Wall -std=gnu18 -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7ve \
          -I$(PREFIX)/include -Ivendor/cjson
LDFLAGS = -L$(PREFIX)/lib -lSDL -lSDL_ttf -lSDL_image -lm -lpthread
surprise: src/*.c vendor/cjson/cJSON.c ; $(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) && $(CROSS_COMPILE)strip $@
```

---

## Decisions and blockers

1. **Launch contract** = Random Game's: write `/mnt/SDCARD/.tmp_update/cmd_to_run.sh` as `LD_PRELOAD=/mnt/SDCARD/miyoo/app/../lib/libpadsp.so "<launch>" "<rompath>"` (fsync), prepend a recents line, `touch /tmp/quick_switch`, exit 0. On cancel: exit 0 without the flag. Don't kill MainUI.
2. **surprise.json** = recents JSONL format, compact, `/../../Roms/` paths, fields < 256 B, no `"` or `:` in values.
3. **Stack** = SDL 1.2 + SDL_ttf + SDL_image from `/mnt/SDCARD/miyoo/lib`, cJSON compiled in. 640×480 with no `full_resolution` file. Classic SDL_Flip path. Optionally have the sync tool pre-scale box art to ≤250×360 to skip rotozoom.
4. **Build** = Colima + union-miyoomini-toolchain built natively for arm64.
5. Open items to check on device: MainUI's exact recents rompath form (duplicate risk in GameSwitcher), background.png orientation, Mini Flip behavior, stock `/customer/lib` contents.
