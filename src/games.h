#ifndef SURPRISE_GAMES_H
#define SURPRISE_GAMES_H

#define MAX_GAMES 64
#define FIELD_MAX 256 /* each field must be < 256 bytes */

typedef struct {
    char label[FIELD_MAX];
    char rompath[FIELD_MAX];
    char imgpath[FIELD_MAX]; /* "" if absent or file missing */
    char launch[FIELD_MAX];
} Game;

/* Load <root>/Roms/surprise.json into out (at most max, capped at MAX_GAMES).
 * Invalid lines and entries whose rompath/launch don't exist are skipped.
 * Returns the count (0 if the file is missing/empty). */
int games_load(Game *out, int max);
#endif
