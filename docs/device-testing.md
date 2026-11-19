# Device testing checklist

Tick each; note failures with `log.txt` attached.

1. Install: extract the zip to the SD root (or `scripts/deploy.sh <host>`). `App/Surprise/` has `surprise`, `launch.sh`, `config.json`, `surprise.png`, `res/`.
2. Generate the list on the card: `scripts/make-surprise-json.py /Volumes/<card> -n 10`. Eject, boot.
3. Apps shows "Surprise" with the gift icon. (Icon-pack caveat: a theme icon pack may override the icon; the PNG basename is unique.)
4. Open it: list shows 10 titles, box art on the right, footer hints and n/total.
5. Navigate: Up/Down, L1/R1 page, wraps/clamps sensibly. Theme matches the active theme; switch theme in Tweaks and reopen.
6. Launch a game with A (and with Start). The game starts. Exit the game: where do you land? Expected: MainUI.
7. Recents and GameSwitcher show the game once. UNVERIFIED: compare the `rompath` form MainUI writes. Launch some other game normally from the game list, then read `/mnt/SDCARD/Roms/recentlist*.json` and compare its `rompath` to our line. If forms differ, GameSwitcher may show duplicates.
8. Play activity: the launched game's play time is counted (Onion's playActivity).
9. B and Menu exit cleanly back to Apps, with nothing launched and recents unchanged.
10. UNVERIFIED: background orientation (`bg-list-s`, `preview-bg`, theme `background.png`) renders upright, not rotated.
11. Mini Plus and Mini Flip: UNVERIFIED on Flip (screen, input, no `full_resolution`).
12. Empty/missing `surprise.json` shows the message; B exits.
13. Logs: `App/Surprise/log.txt` (stdout+stderr of the last run, truncated each run). Over SSH (Tweaks → Network; root, blank password):

```sh
ssh root@<host> 'cat /mnt/SDCARD/App/Surprise/log.txt'
ssh root@<host> 'cat /mnt/SDCARD/.tmp_update/cmd_to_run.sh; head -c 600 /mnt/SDCARD/Roms/recentlist*.json'
scripts/deploy.sh <host> --run     # redeploy, run, print log
```
