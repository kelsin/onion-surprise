#include "theme.h"
#include "paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <SDL/SDL_image.h>
#include "cJSON.h"

static cJSON *read_json(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    cJSON *j = NULL;
    if (n > 0 && n < (1 << 20)) {
        char *buf = malloc(n + 1);
        if (buf && fread(buf, 1, n, f) == (size_t)n) {
            buf[n] = 0;
            j = cJSON_Parse(buf);
        }
        free(buf);
    }
    fclose(f);
    return j;
}

static int is_dir(const char *p) {
    struct stat st;
    return stat(p, &st) == 0 && S_ISDIR(st.st_mode);
}

/* Theme dir (mapped, with trailing '/') from <root>/system.json, else <root>/miyoo/app/. */
static void theme_dir(char *out, size_t n) {
    char p[1000];
    paths_card(p, sizeof p, "/system.json");
    cJSON *sys = read_json(p);
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(sys, "theme");
    out[0] = 0;
    if (cJSON_IsString(v) && strcmp(v->valuestring, "./") != 0) {
        paths_map(p, sizeof p, v->valuestring);
        if (is_dir(p)) snprintf(out, n, "%s%s", p, p[strlen(p) - 1] == '/' ? "" : "/");
    }
    cJSON_Delete(sys);
    if (!out[0]) paths_card(out, n, "/miyoo/app/");
}

/* section.key from the profile override, then the theme config. */
static const cJSON *cfg_get(const cJSON *over, const cJSON *base, const char *sec, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(over, sec), key);
    if (!v) v = cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(base, sec), key);
    return v;
}

static const char *cfg_str(const cJSON *over, const cJSON *base, const char *sec, const char *fallback_sec, const char *key) {
    const cJSON *v = cfg_get(over, base, sec, key);
    if (!cJSON_IsString(v) && fallback_sec) v = cfg_get(over, base, fallback_sec, key);
    return cJSON_IsString(v) && v->valuestring[0] ? v->valuestring : NULL;
}

static SDL_Color parse_color(const char *s) {
    SDL_Color c = {255, 255, 255, 0};
    unsigned int rgb;
    if (s && s[0] == '#' && strlen(s) == 7 && sscanf(s + 1, "%6x", &rgb) == 1) {
        c.r = (rgb >> 16) & 0xff;
        c.g = (rgb >> 8) & 0xff;
        c.b = rgb & 0xff;
    }
    return c;
}

static TTF_Font *open_font(const char *dir, const char *name, int size, const char *bundled) {
    char p[1024], alt[1024];
    TTF_Font *f = NULL;
    if (name) {
        if (name[0] == '/') paths_map(p, sizeof p, name);
        else snprintf(p, sizeof p, "%s%s", dir, name);
        f = TTF_OpenFont(p, size);
    }
    paths_card(alt, sizeof alt, "/miyoo/app/wqy-microhei.ttc");
    if (!f) f = TTF_OpenFont(alt, size);
    if (!f) f = TTF_OpenFont("/customer/app/Exo-2-Bold-Italic.ttf", size);
    if (!f && bundled) f = TTF_OpenFont(bundled, size);
    return f;
}

static void load_font(ThemeFont *tf, const cJSON *over, const cJSON *base, const char *dir,
                      const char *sec, const char *fallback_sec, int def_size, const char *bundled) {
    const cJSON *sz = cfg_get(over, base, sec, "size");
    if (!(cJSON_IsNumber(sz) && sz->valueint > 0) && fallback_sec)
        sz = cfg_get(over, base, fallback_sec, "size");
    int size = cJSON_IsNumber(sz) && sz->valueint > 0 ? sz->valueint : def_size;
    tf->font = open_font(dir, cfg_str(over, base, sec, fallback_sec, "font"), size, bundled);
    tf->color = parse_color(cfg_str(over, base, sec, fallback_sec, "color"));
}

static SDL_Surface *load_skin(const char *dir, const char *name) {
    char p[1024];
    char rel[128];
    snprintf(rel, sizeof rel, "/Saves/CurrentProfile/theme/skin/%s.png", name);
    paths_card(p, sizeof p, rel);
    SDL_Surface *s = access(p, R_OK) == 0 ? IMG_Load(p) : NULL;
    if (!s) {
        snprintf(p, sizeof p, "%sskin/%s.png", dir, name);
        if (access(p, R_OK) == 0) s = IMG_Load(p);
    }
    if (!s) {
        snprintf(rel, sizeof rel, "/miyoo/app/skin/%s.png", name);
        paths_card(p, sizeof p, rel);
        if (access(p, R_OK) == 0) s = IMG_Load(p);
    }
    if (!s) return NULL;
    SDL_Surface *d = SDL_DisplayFormatAlpha(s);
    if (d) {
        SDL_FreeSurface(s);
        return d;
    }
    return s;
}

void theme_load(Theme *t, const char *bundled_font) {
    char dir[1024], p[1100];
    memset(t, 0, sizeof *t);
    theme_dir(dir, sizeof dir);

    snprintf(p, sizeof p, "%sconfig.json", dir);
    cJSON *base = read_json(p);
    if (!base) {
        paths_card(p, sizeof p, "/miyoo/app/config.json");
        base = read_json(p);
    }
    paths_card(p, sizeof p, "/Saves/CurrentProfile/theme/config.json");
    cJSON *over = read_json(p);

    load_font(&t->title, over, base, dir, "title", NULL, 36, bundled_font);
    load_font(&t->list, over, base, dir, "list", "title", 24, bundled_font);
    load_font(&t->hint, over, base, dir, "hint", "title", 40, bundled_font);
    if (t->list.font) TTF_SetFontStyle(t->list.font, TTF_STYLE_BOLD);
    cJSON_Delete(base);
    cJSON_Delete(over);

    t->background = load_skin(dir, "background");
    t->bg_title = load_skin(dir, "bg-title");
    t->bg_list_s = load_skin(dir, "bg-list-s");
    t->div_line_h = load_skin(dir, "div-line-h");
    t->tips_bar_bg = load_skin(dir, "tips-bar-bg");
    t->preview_bg = load_skin(dir, "preview-bg");
    t->icon_a = load_skin(dir, "icon-A-54");
    t->icon_b = load_skin(dir, "icon-B-54");
}

void theme_free(Theme *t) {
    ThemeFont *fonts[] = {&t->title, &t->list, &t->hint};
    for (int i = 0; i < 3; i++)
        if (fonts[i]->font) TTF_CloseFont(fonts[i]->font);
    SDL_Surface *imgs[] = {t->background, t->bg_title, t->bg_list_s, t->div_line_h,
                           t->tips_bar_bg, t->preview_bg, t->icon_a, t->icon_b};
    for (size_t i = 0; i < sizeof imgs / sizeof *imgs; i++)
        if (imgs[i]) SDL_FreeSurface(imgs[i]);
    memset(t, 0, sizeof *t);
}
