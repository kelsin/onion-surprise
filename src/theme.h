#ifndef SURPRISE_THEME_H
#define SURPRISE_THEME_H
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>

typedef struct {
    TTF_Font *font; /* NULL if no font could be opened */
    SDL_Color color;
} ThemeFont;

typedef struct {
    ThemeFont title, list, hint;
    /* Skin images; NULL if missing (callers draw solid-colour fallbacks). */
    SDL_Surface *background, *bg_title, *bg_list_s, *div_line_h, *tips_bar_bg, *preview_bg, *icon_a, *icon_b;
} Theme;

/* Load the current Onion theme (system.json "theme"). Needs TTF_Init and a video mode.
 * bundled_font is the app's own fallback TTF. Never fails; missing parts stay NULL. */
void theme_load(Theme *t, const char *bundled_font);
void theme_free(Theme *t);
#endif
