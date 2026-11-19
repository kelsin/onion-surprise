#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <SDL/SDL_image.h>

#define HEADER_H 60
#define ROW_H 60
#define LIST_Y HEADER_H
#define FOOTER_Y (LIST_Y + UI_ROWS * ROW_H) /* 420 */
#define FOOTER_H 60
#define LABEL_X 20
#define PREVIEW_MAX_W 250
#define PANEL_W 256 /* default preview-bg size, used when the skin is missing */
#define PANEL_H 360
#define EMPTY_MSG "No surprise games yet \xe2\x80\x94 run sync-roms"

/* Solid-colour fallbacks when skin images are missing. */
static void fill(SDL_Surface *d, int x, int y, int w, int h, Uint8 r, Uint8 g, Uint8 b) {
    SDL_Rect rc = {x, y, w, h};
    SDL_FillRect(d, &rc, SDL_MapRGB(d->format, r, g, b));
}

static void blit(SDL_Surface *s, SDL_Surface *d, int x, int y) {
    SDL_Rect rc = {x, y, 0, 0};
    SDL_BlitSurface(s, NULL, d, &rc);
}

/* align: 0 left at x, 1 centered on x, 2 right edge at x. y is the vertical centre. Returns width. */
static int text(SDL_Surface *d, const ThemeFont *f, const char *s, int x, int y, int align) {
    if (!f->font || !s[0]) return 0;
    SDL_Surface *ts = TTF_RenderUTF8_Blended(f->font, s, f->color);
    if (!ts) return 0;
    int w = ts->w;
    if (align == 1) x -= w / 2;
    else if (align == 2) x -= w;
    blit(ts, d, x, y - ts->h / 2);
    SDL_FreeSurface(ts);
    return w;
}

/* Copy s into out, cut with "..." so it renders at most max_w wide. */
static void fit_text(TTF_Font *font, const char *s, int max_w, char *out, size_t n) {
    int w = 0;
    snprintf(out, n, "%s", s);
    if (!font || TTF_SizeUTF8(font, out, &w, NULL) != 0 || w <= max_w) return;
    size_t len = strlen(out);
    while (len > 0) {
        do len--; while (len > 0 && ((unsigned char)s[len] & 0xC0) == 0x80); /* stay on a UTF-8 boundary */
        while (len > 0 && s[len - 1] == ' ') len--;
        snprintf(out, n, "%.*s...", (int)len, s);
        if (TTF_SizeUTF8(font, out, &w, NULL) == 0 && w <= max_w) return;
    }
    snprintf(out, n, "...");
}

static int panel_w(const Theme *t) { return t->preview_bg ? t->preview_bg->w : PANEL_W; }
static int panel_h(const Theme *t) { return t->preview_bg ? t->preview_bg->h : PANEL_H; }

SDL_Surface *ui_scale_fit(SDL_Surface *src, int max_w, int max_h) {
    SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, src->w, src->h, 32,
                                          0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    if (!s) return NULL;
    SDL_Surface *a = SDL_ConvertSurface(src, s->format, SDL_SWSURFACE); /* copies alpha as-is */
    SDL_FreeSurface(s);
    if (!a) return NULL;

    int sw = a->w, sh = a->h, dw = sw, dh = sh;
    if (dw > max_w) { dh = (int)((long)dh * max_w / dw); dw = max_w; }
    if (dh > max_h) { dw = (int)((long)dw * max_h / dh); dh = max_h; }
    if (dw < 1) dw = 1;
    if (dh < 1) dh = 1;
    if (dw == sw && dh == sh) return a;

    SDL_Surface *d = SDL_CreateRGBSurface(SDL_SWSURFACE, dw, dh, 32,
                                          0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    if (!d) {
        SDL_FreeSurface(a);
        return NULL;
    }
    SDL_LockSurface(a);
    SDL_LockSurface(d);
    /* Box filter: each destination pixel averages its source footprint (alpha-weighted). */
    for (int y = 0; y < dh; y++) {
        int y0 = (int)((long)y * sh / dh), y1 = (int)((long)(y + 1) * sh / dh);
        if (y1 <= y0) y1 = y0 + 1;
        Uint32 *drow = (Uint32 *)((Uint8 *)d->pixels + y * d->pitch);
        for (int x = 0; x < dw; x++) {
            int x0 = (int)((long)x * sw / dw), x1 = (int)((long)(x + 1) * sw / dw);
            if (x1 <= x0) x1 = x0 + 1;
            unsigned long r = 0, g = 0, b = 0, al = 0, cnt = 0;
            for (int yy = y0; yy < y1; yy++) {
                const Uint32 *srow = (const Uint32 *)((const Uint8 *)a->pixels + yy * a->pitch);
                for (int xx = x0; xx < x1; xx++) {
                    Uint32 p = srow[xx];
                    unsigned long pa = p >> 24;
                    r += ((p >> 16) & 0xff) * pa;
                    g += ((p >> 8) & 0xff) * pa;
                    b += (p & 0xff) * pa;
                    al += pa;
                    cnt++;
                }
            }
            Uint32 out = 0;
            if (al) out = ((Uint32)(al / cnt) << 24) | ((Uint32)(r / al) << 16) | ((Uint32)(g / al) << 8) | (Uint32)(b / al);
            drow[x] = out;
        }
    }
    SDL_UnlockSurface(d);
    SDL_UnlockSurface(a);
    SDL_FreeSurface(a);
    return d;
}

SDL_Surface *ui_load_art(const Theme *t, const char *path) {
    SDL_Surface *img = IMG_Load(path);
    if (!img) return NULL;
    int mw = panel_w(t) < PREVIEW_MAX_W ? panel_w(t) : PREVIEW_MAX_W;
    int mh = panel_h(t) < PANEL_H ? panel_h(t) : PANEL_H;
    SDL_Surface *s = ui_scale_fit(img, mw, mh);
    SDL_FreeSurface(img);
    return s; /* software ARGB on purpose: see load_skin in theme.c */
}

static void hint(SDL_Surface *d, const Theme *t, SDL_Surface *icon, const char *letter, const char *label, int *x) {
    int cy = FOOTER_Y + FOOTER_H / 2;
    if (icon) {
        blit(icon, d, *x, cy - icon->h / 2);
        *x += icon->w;
    } else {
        fill(d, *x + 6, cy - 18, 36, 36, 90, 90, 110);
        text(d, &t->hint, letter, *x + 24, cy, 1);
        *x += 48;
    }
    *x += 6 + text(d, &t->hint, label, *x + 6, cy, 0) + 24;
}

void ui_render(SDL_Surface *d, const Theme *t, const Game *games, int count, int sel, int top, SDL_Surface *art) {
    int W = d->w;
    char buf[FIELD_MAX + 8];

    fill(d, 0, 0, W, d->h, 24, 24, 32);
    if (t->background) blit(t->background, d, 0, 0);

    /* Header */
    if (t->bg_title) blit(t->bg_title, d, 0, 0);
    else if (!t->background) fill(d, 0, 0, W, HEADER_H, 36, 36, 48);
    text(d, &t->title, "Surprise", W / 2, HEADER_H / 2, 1);

    if (count == 0) {
        fit_text(t->list.font, EMPTY_MSG, W - 40, buf, sizeof buf);
        text(d, &t->list, buf, W / 2, LIST_Y + UI_ROWS * ROW_H / 2, 1);
    } else {
        int pw = panel_w(t);
        int label_w = W - pw - 20 - LABEL_X;
        for (int i = 0; i < UI_ROWS && top + i < count; i++) {
            int y = LIST_Y + i * ROW_H;
            if (top + i == sel) {
                if (t->bg_list_s) blit(t->bg_list_s, d, 0, y);
                else fill(d, 0, y, W, ROW_H, 60, 70, 100);
            }
            if (i < UI_ROWS - 1) {
                if (t->div_line_h) blit(t->div_line_h, d, 0, y + ROW_H - t->div_line_h->h / 2);
                else fill(d, 0, y + ROW_H - 1, W, 1, 54, 54, 70);
            }
            fit_text(t->list.font, games[top + i].label, label_w, buf, sizeof buf);
            text(d, &t->list, buf, LABEL_X, y + ROW_H / 2, 0);
        }
        if (art) {
            if (t->preview_bg) blit(t->preview_bg, d, W - pw, LIST_Y);
            else fill(d, W - pw, LIST_Y, pw, PANEL_H, 40, 40, 52);
            blit(art, d, W - pw + (pw - art->w) / 2, 240 - art->h / 2);
        }
    }

    /* Footer */
    if (t->tips_bar_bg) blit(t->tips_bar_bg, d, 0, FOOTER_Y);
    else if (!t->background) fill(d, 0, FOOTER_Y, W, FOOTER_H, 36, 36, 48);
    int x = 10;
    if (count > 0) hint(d, t, t->icon_a, "A", "Launch", &x);
    hint(d, t, t->icon_b, "B", "Back", &x);
    if (count > 0) {
        snprintf(buf, sizeof buf, "%d/%d", sel + 1, count);
        text(d, &t->hint, buf, W - 20, FOOTER_Y + FOOTER_H / 2, 2);
    }
}
