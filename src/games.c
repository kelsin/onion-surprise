#include "games.h"
#include "paths.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"

#define LINE_MAX_BYTES 1024

/* Valid value: non-empty, < 256 bytes, no '"', ':', '\\' or control chars. */
static int value_ok(const char *s) {
    size_t n = strlen(s);
    if (n == 0 || n >= FIELD_MAX) return 0;
    for (; *s; s++)
        if (*s == '"' || *s == ':' || *s == '\\' || (unsigned char)*s < 0x20) return 0;
    return 1;
}

static const char *get_str(const cJSON *o, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
    return cJSON_IsString(v) ? v->valuestring : NULL;
}

static int exists_mapped(const char *entry_path) {
    char p[1024];
    paths_map(p, sizeof p, entry_path);
    return access(p, F_OK) == 0;
}

static int parse_line(const char *line, Game *g) {
    cJSON *o = cJSON_Parse(line);
    int ok = 0;
    if (!cJSON_IsObject(o)) goto done;
    const char *label = get_str(o, "label"), *rom = get_str(o, "rompath"),
               *launch = get_str(o, "launch"), *img = get_str(o, "imgpath");
    if (!label || !rom || !launch || !value_ok(label) || !value_ok(rom) || !value_ok(launch)) goto done;
    if (cJSON_GetObjectItemCaseSensitive(o, "imgpath") && (!img || !value_ok(img))) goto done;
    if (!exists_mapped(rom) || !exists_mapped(launch)) goto done;
    memset(g, 0, sizeof *g);
    snprintf(g->label, FIELD_MAX, "%s", label);
    snprintf(g->rompath, FIELD_MAX, "%s", rom);
    snprintf(g->launch, FIELD_MAX, "%s", launch);
    if (img && exists_mapped(img)) snprintf(g->imgpath, FIELD_MAX, "%s", img);
    ok = 1;
done:
    cJSON_Delete(o);
    return ok;
}

int games_load(Game *out, int max) {
    char path[1024], buf[LINE_MAX_BYTES + 2];
    if (max > MAX_GAMES) max = MAX_GAMES;
    paths_card(path, sizeof path, "/Roms/surprise.json");
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    int n = 0, too_long = 0;
    while (n < max && fgets(buf, sizeof buf, f)) {
        size_t len = strlen(buf);
        int has_nl = len && buf[len - 1] == '\n';
        if (has_nl) buf[--len] = 0;
        if (!has_nl && !feof(f)) too_long = 1;      /* line continues past buffer */
        if (len >= LINE_MAX_BYTES) too_long = 1;
        if (too_long) {                              /* drain rest of this line */
            while (!has_nl && fgets(buf, sizeof buf, f)) {
                size_t l = strlen(buf);
                has_nl = l && buf[l - 1] == '\n';
            }
            too_long = 0;
            continue;
        }
        if (len && buf[len - 1] == '\r') buf[--len] = 0;
        if (len == 0) continue;
        if (parse_line(buf, &out[n])) n++;
    }
    fclose(f);
    return n;
}
