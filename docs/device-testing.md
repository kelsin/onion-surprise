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
13. Launch via Surprise 3 times in a row: no error flash or fatal panel.
14. MENU inside the app exits it (keymon does not intercept).
15. Power-button suspend/resume inside the app, then A still launches.
16. After a game exits, note which MainUI tab you land on. Reopen Surprise, launch a different game, and confirm recents has both, newest first, without duplicates. After several launches, `wc -l` the recents file to check it does not grow unexpectedly.
17. CJK/accented labels render with only the bundled font.
18. Footer hint size on a theme without a `hint` section (should follow `title`).
19. Scrolling lag with large (e.g. 500x700) box art.
20. `scripts/deploy.sh <host>` works against dropbear (single ssh connection, tar over stdin).
21. `log.txt` is empty after a normal run; any `SDL_Init`, `video:` or `launch failed:` line points at the failure.
22. Logs: `App/Surprise/log.txt` (stdout+stderr of the last run, truncated each run). Over SSH (Tweaks → Network; root, blank password):

```sh
ssh root@<host> 'cat /mnt/SDCARD/App/Surprise/log.txt'
ssh root@<host> 'cat /mnt/SDCARD/.tmp_update/cmd_to_run.sh; head -c 600 /mnt/SDCARD/Roms/recentlist*.json'
scripts/deploy.sh <host> --run     # redeploy, run, print log
```
