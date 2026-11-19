#ifndef SURPRISE_LAUNCH_H
#define SURPRISE_LAUNCH_H
#include <stddef.h>
#include "games.h"

/* The cmd_to_run.sh contents (no trailing newline). */
void launch_build_cmd(char *out, size_t n, const Game *g);
/* Compact recents line: label,rompath,imgpath,launch,type (imgpath omitted if empty), no newline. */
void launch_build_line(char *out, size_t n, const Game *g);

/* Full launch contract: write cmd_to_run.sh, prepend recents, touch quick_switch, sync.
 * Returns 0 on success, -1 if cmd_to_run.sh could not be written (nothing else touched). */
int launch_game(const Game *g);
#endif
