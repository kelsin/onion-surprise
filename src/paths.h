#ifndef SURPRISE_PATHS_H
#define SURPRISE_PATHS_H
#include <stddef.h>

#define CARD_PREFIX "/mnt/SDCARD"

/* Read SURPRISE_SDCARD / SURPRISE_TMP from the environment (call once, again after changing env). */
void paths_init(void);
const char *paths_root(void); /* card root, default /mnt/SDCARD */
const char *paths_tmp(void);  /* default /tmp */

/* out = root + suffix, e.g. paths_card(buf, n, "/Roms/surprise.json"). */
void paths_card(char *out, size_t n, const char *suffix);

/* Map an entry path (as written in surprise.json) to its on-disk path:
 * a leading /mnt/SDCARD is replaced by the root; other paths are copied. */
void paths_map(char *out, size_t n, const char *entry_path);
#endif
