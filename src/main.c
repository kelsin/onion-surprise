#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include "paths.h"
#include "games.h"
#include "launch.h"
#include "theme.h"
#include "ui.h"

static volatile sig_atomic_t g_quit;
static void on_signal(int sig) { (void)sig; g_quit = 1; }

static Game g_games[MAX_GAMES];

/* <dir of this executable>/res/font.ttf, falling back to ./res/font.ttf. */
static void bundled_font(char *out, size_t n) {
    char exe[1024];
    ssize_t len = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (len > 0) {
        exe[len] = 0;
        char *slash = strrchr(exe, '/');
        if (slash) {
            *slash = 0;
            snprintf(out, n, "%s/res/font.ttf", exe);
            if (access(out, R_OK) == 0) return;
        }
    }
    snprintf(out, n, "res/font.ttf");
}

static SDL_Surface *load_art(const Theme *t, const Game *g) {
    char p[1024];
    if (!g->imgpath[0]) return NULL;
    paths_map(p, sizeof p, g->imgpath);
    return ui_load_art(t, p);
}

int main(int argc, char **argv) {
    const char *shot = NULL;
    int sel = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) shot = argv[++i];
        else if (strcmp(argv[i], "--select") == 0 && i + 1 < argc) sel = atoi(argv[++i]);
    }

    paths_init();
    int count = games_load(g_games, MAX_GAMES);
    if (sel < 0 || sel >= count) sel = 0;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 0;
    }
    signal(SIGTERM, on_signal);
    signal(SIGINT, on_signal);
    SDL_ShowCursor(SDL_DISABLE);
    SDL_EnableKeyRepeat(300, 50);
    if (TTF_Init() < 0) fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());

    SDL_Surface *video = SDL_SetVideoMode(640, 480, 32, SDL_HWSURFACE);
    SDL_Surface *screen = video ? SDL_CreateRGBSurface(SDL_HWSURFACE, video->w, video->h, 32, 0, 0, 0, 0) : NULL;
    if (!screen) {
        fprintf(stderr, "video: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    char font[1100];
    bundled_font(font, sizeof font);
    Theme theme;
    theme_load(&theme, font);

    int top = sel >= UI_ROWS ? sel - UI_ROWS + 1 : 0;
    int art_for = -1, dirty = 1, launch = 0;
    SDL_Surface *art = NULL;

    while (!g_quit) {
        if (count > 0 && art_for != sel) {
            if (art) SDL_FreeSurface(art);
            art = load_art(&theme, &g_games[sel]);
            art_for = sel;
        }
        if (dirty) {
            ui_render(screen, &theme, g_games, count, sel, top, art);
            SDL_BlitSurface(screen, NULL, video, NULL);
            SDL_Flip(video);
            dirty = 0;
            if (shot) {
                if (SDL_SaveBMP(screen, shot) != 0) fprintf(stderr, "SaveBMP: %s\n", SDL_GetError());
                break;
            }
        }

        SDL_Event ev;
        while (!g_quit && SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                g_quit = 1;
                break;
            }
            if (ev.type != SDL_KEYDOWN) continue;
            int prev = sel;
            switch (ev.key.keysym.sym) {
            case SDLK_LCTRL:  /* B */
            case SDLK_ESCAPE: /* MENU */
                g_quit = 1;
                break;
            case SDLK_SPACE:  /* A */
            case SDLK_RETURN: /* START */
                if (count > 0) launch = g_quit = 1;
                break;
            case SDLK_UP:
                if (count > 0) sel = sel > 0 ? sel - 1 : count - 1;
                break;
            case SDLK_DOWN:
                if (count > 0) sel = sel < count - 1 ? sel + 1 : 0;
                break;
            case SDLK_LEFT:
            case SDLK_e: /* L1 */
                sel = sel >= UI_ROWS ? sel - UI_ROWS : 0;
                break;
            case SDLK_RIGHT:
            case SDLK_t: /* R1 */
                if (count > 0) sel = sel + UI_ROWS < count ? sel + UI_ROWS : count - 1;
                break;
            default:
                break;
            }
            if (sel != prev) {
                if (sel < top) top = sel;
                else if (sel >= top + UI_ROWS) top = sel - UI_ROWS + 1;
                dirty = 1;
            }
        }
        if (!g_quit && !dirty) SDL_Delay(16);
    }

    if (art) SDL_FreeSurface(art);
    theme_free(&theme);
    SDL_FreeSurface(screen);
    TTF_Quit();
    SDL_Quit();

    /* On failure there is nothing useful to show; MainUI simply returns. */
    if (launch && launch_game(&g_games[sel]) != 0) fprintf(stderr, "launch failed: %s\n", g_games[sel].rompath);
    return 0;
}
