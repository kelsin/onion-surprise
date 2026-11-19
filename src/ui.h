#ifndef SURPRISE_UI_H
#define SURPRISE_UI_H
#include <SDL/SDL.h>
#include "theme.h"
#include "games.h"

#define UI_ROWS 6

/* Downscale src (any format) to fit max_w x max_h keeping aspect; never upscales.
 * Returns a new surface (caller frees) or NULL. Does not free src. */
SDL_Surface *ui_scale_fit(SDL_Surface *src, int max_w, int max_h);

/* Load box art from an on-disk path and scale it for the preview panel. NULL on failure. */
SDL_Surface *ui_load_art(const Theme *t, const char *path);

/* Draw one frame. count == 0 draws the empty state. top is the first visible row;
 * art is the (already scaled) box art of the selected game, or NULL. */
void ui_render(SDL_Surface *dst, const Theme *t, const Game *games, int count, int sel, int top, SDL_Surface *art);
#endif
