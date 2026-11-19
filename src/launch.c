#include "launch.h"
#include "paths.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void launch_build_cmd(char *out, size_t n, const Game *g) {
    snprintf(out, n, "LD_PRELOAD=/mnt/SDCARD/miyoo/app/../lib/libpadsp.so \"%s\" \"%s\"", g->launch, g->rompath);
}

void launch_build_line(char *out, size_t n, const Game *g) {
    int w = snprintf(out, n, "{\"label\":\"%s\",\"rompath\":\"%s\"", g->label, g->rompath);
    if (g->imgpath[0] && w > 0 && (size_t)w < n)
        w += snprintf(out + w, n - w, ",\"imgpath\":\"%s\"", g->imgpath);
    if (w > 0 && (size_t)w < n)
        snprintf(out + w, n - w, ",\"launch\":\"%s\",\"type\":5}", g->launch);
}

static int write_sync(const char *path, const char *data) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    int ok = fputs(data, f) >= 0 && fflush(f) == 0 && fsync(fileno(f)) == 0;
    return (fclose(f) == 0 && ok) ? 0 : -1;
}

/* Extract the rompath value of a recents line into out; returns 0 if absent. */
static int line_rompath(const char *line, char *out, size_t n) {
    const char *k = "\"rompath\":\"";
    const char *s = strstr(line, k);
    if (!s) return 0;
    s += strlen(k);
    const char *e = strchr(s, '"');
    if (!e || (size_t)(e - s) >= n) return 0;
    memcpy(out, s, e - s);
    out[e - s] = 0;
    return 1;
}

static void update_recents(const Game *g) {
    char path[1024], tmp[1100], cfg[1024], ours[1200], line[2048], rp[FIELD_MAX];
    paths_card(cfg, sizeof cfg, "/.tmp_update/config/.showRecents");
    paths_card(path, sizeof path, access(cfg, F_OK) == 0 ? "/Roms/recentlist.json" : "/Roms/recentlist-hidden.json");
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    launch_build_line(ours, sizeof ours, g);

    FILE *out = fopen(tmp, "w");
    if (!out) return;
    fprintf(out, "%s\n", ours);
    FILE *in = fopen(path, "r");
    if (in) {
        while (fgets(line, sizeof line, in)) {
            if (line[0] == '\n' || line[0] == 0) continue;
            if (line_rompath(line, rp, sizeof rp) && strcmp(rp, g->rompath) == 0) continue;
            fputs(line, out);
            if (!strchr(line, '\n')) fputc('\n', out);
        }
        fclose(in);
    }
    fflush(out);
    fsync(fileno(out));
    if (fclose(out) != 0 || rename(tmp, path) != 0) unlink(tmp);
}

int launch_game(const Game *g) {
    char cmd[1200], path[1024];
    launch_build_cmd(cmd, sizeof cmd, g);
    paths_card(path, sizeof path, "/.tmp_update/cmd_to_run.sh");
    if (write_sync(path, cmd) != 0) return -1;
    update_recents(g);
    snprintf(path, sizeof path, "%s/quick_switch", paths_tmp());
    FILE *q = fopen(path, "w");
    if (q) fclose(q);
    sync();
    return 0;
}
