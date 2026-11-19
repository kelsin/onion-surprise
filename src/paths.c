#include "paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char g_root[256] = CARD_PREFIX;
static char g_tmp[256] = "/tmp";

static void from_env(char *dst, size_t n, const char *name, const char *def) {
    const char *v = getenv(name);
    snprintf(dst, n, "%s", (v && *v) ? v : def);
}

void paths_init(void) {
    from_env(g_root, sizeof g_root, "SURPRISE_SDCARD", CARD_PREFIX);
    from_env(g_tmp, sizeof g_tmp, "SURPRISE_TMP", "/tmp");
}

const char *paths_root(void) { return g_root; }
const char *paths_tmp(void) { return g_tmp; }

void paths_card(char *out, size_t n, const char *suffix) {
    snprintf(out, n, "%s%s", g_root, suffix);
}

void paths_map(char *out, size_t n, const char *p) {
    size_t pl = strlen(CARD_PREFIX);
    if (strncmp(p, CARD_PREFIX, pl) == 0 && (p[pl] == '/' || p[pl] == 0))
        snprintf(out, n, "%s%s", g_root, p + pl);
    else
        snprintf(out, n, "%s", p);
}
