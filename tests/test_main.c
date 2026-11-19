#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "paths.h"
#include "games.h"
#include "launch.h"

static int failures, checks;
#define CHECK(c) do { checks++; if (!(c)) { failures++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); } } while (0)

static char root[512], tmpd[512];

static void wr(const char *rel, const char *data) {
    char p[1024]; snprintf(p, sizeof p, "%s%s", root, rel);
    FILE *f = fopen(p, "w"); if (!f) { printf("cannot write %s\n", p); exit(2); }
    fputs(data, f); fclose(f);
}
static void touchf(const char *rel) { wr(rel, ""); }
static char *slurp(const char *rel, int absolute) {
    char p[1024]; if (absolute) snprintf(p, sizeof p, "%s", rel); else snprintf(p, sizeof p, "%s%s", root, rel);
    FILE *f = fopen(p, "r"); if (!f) return NULL;
    static char buf[8192]; size_t n = fread(buf, 1, sizeof buf - 1, f); buf[n] = 0; fclose(f); return buf;
}
static int exists(const char *rel) { char p[1024]; snprintf(p, sizeof p, "%s%s", root, rel); return access(p, F_OK) == 0; }

static void setup(void) {
    char t[] = "/tmp/surprise-test-XXXXXX";
    if (!mkdtemp(t)) exit(2);
    snprintf(root, sizeof root, "%s/card", t); snprintf(tmpd, sizeof tmpd, "%s/tmp", t);
    mkdir(root, 0755); mkdir(tmpd, 0755);
    char p[600];
    const char *dirs[] = {"/Roms", "/Roms/GBA", "/Roms/GBA/Imgs", "/Emu", "/Emu/GBA", "/.tmp_update", "/.tmp_update/config"};
    for (size_t i = 0; i < sizeof dirs / sizeof *dirs; i++) { snprintf(p, sizeof p, "%s%s", root, dirs[i]); mkdir(p, 0755); }
    setenv("SURPRISE_SDCARD", root, 1); setenv("SURPRISE_TMP", tmpd, 1);
    paths_init();
    touchf("/Emu/GBA/launch.sh"); touchf("/Roms/GBA/Metroid.gba"); touchf("/Roms/GBA/My Game.gba");
}

#define ROM(f) "/mnt/SDCARD/Emu/GBA/../../Roms/GBA/" f
#define LAUNCH "/mnt/SDCARD/Emu/GBA/launch.sh"
#define LINE(l, r, i, la) "{\"label\":\"" l "\",\"rompath\":\"" r "\"" i ",\"launch\":\"" la "\",\"type\":5}\n"

static void test_paths(void) {
    char b[600];
    CHECK(strcmp(paths_root(), root) == 0);
    paths_map(b, sizeof b, "/mnt/SDCARD/Emu/GBA/../../Roms/x.gba");
    char want[700]; snprintf(want, sizeof want, "%s/Emu/GBA/../../Roms/x.gba", root);
    CHECK(strcmp(b, want) == 0);
    paths_map(b, sizeof b, "/other/x"); CHECK(strcmp(b, "/other/x") == 0);
    paths_card(b, sizeof b, "/Roms/surprise.json"); snprintf(want, sizeof want, "%s/Roms/surprise.json", root); CHECK(strcmp(b, want) == 0);
}

static void test_games(void) {
    Game g[MAX_GAMES];
    /* missing file */
    CHECK(games_load(g, MAX_GAMES) == 0);

    touchf("/Roms/GBA/Imgs/Metroid.png");
    char longv[300]; memset(longv, 'a', 256); longv[256] = 0;
    char l255[300]; memset(l255, 'b', 255); l255[255] = 0;
    char data[8192] = "";
    strcat(data, LINE("Metroid", ROM("Metroid.gba"), ",\"imgpath\":\"/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Imgs/Metroid.png\"", LAUNCH));
    strcat(data, LINE("Spaced", ROM("My Game.gba"), "", LAUNCH));                 /* spaces + no imgpath */
    strcat(data, "\n");                                                              /* blank line */
    strcat(data, LINE("Col:on", ROM("Metroid.gba"), "", LAUNCH));                   /* ':' in value */
    strcat(data, "{\"label\":\"Q\\\"uote\",\"rompath\":\"" ROM("Metroid.gba") "\",\"launch\":\"" LAUNCH "\",\"type\":5}\n");
    strcat(data, "{\"label\":\"");  strcat(data, longv); strcat(data, "\",\"rompath\":\"" ROM("Metroid.gba") "\",\"launch\":\"" LAUNCH "\",\"type\":5}\n"); /* 256B */
    strcat(data, "{\"label\":\"");  strcat(data, l255);  strcat(data, "\",\"rompath\":\"" ROM("Metroid.gba") "\",\"launch\":\"" LAUNCH "\",\"type\":5}\n"); /* 255B ok */
    strcat(data, LINE("NoRom", ROM("Nope.gba"), "", LAUNCH));                       /* missing rom */
    strcat(data, LINE("NoLaunch", ROM("Metroid.gba"), "", "/mnt/SDCARD/Emu/ZZZ/launch.sh"));
    strcat(data, LINE("MissingImg", ROM("Metroid.gba"), ",\"imgpath\":\"/mnt/SDCARD/Roms/GBA/Imgs/none.png\"", LAUNCH));
    strcat(data, "not json at all\n");
    strcat(data, "{\"label\":\"NoLaunchKey\",\"rompath\":\"" ROM("Metroid.gba") "\"}\n");
    wr("/Roms/surprise.json", data);

    int n = games_load(g, MAX_GAMES);
    CHECK(n == 4);
    if (n == 4) {
        CHECK(strcmp(g[0].label, "Metroid") == 0);
        CHECK(strcmp(g[0].rompath, ROM("Metroid.gba")) == 0);
        CHECK(strcmp(g[0].imgpath, "/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Imgs/Metroid.png") == 0);
        CHECK(strcmp(g[0].launch, LAUNCH) == 0);
        CHECK(strcmp(g[1].label, "Spaced") == 0 && strcmp(g[1].rompath, ROM("My Game.gba")) == 0);
        CHECK(g[1].imgpath[0] == 0);
        CHECK(strlen(g[2].label) == 255);
        CHECK(strcmp(g[3].label, "MissingImg") == 0 && g[3].imgpath[0] == 0);
    }
    CHECK(games_load(g, 2) == 2);

    /* line >= 1024 bytes rejected, following line still read */
    char big[2400]; strcpy(big, "{\"label\":\"x\",\"rompath\":\"" ROM("Metroid.gba") "\",\"launch\":\"" LAUNCH "\",\"pad\":\"");
    size_t k = strlen(big); memset(big + k, 'p', 1100); big[k + 1100] = 0; strcat(big, "\"}\n");
    strcat(big, LINE("After", ROM("Metroid.gba"), "", LAUNCH));
    wr("/Roms/surprise.json", big);
    n = games_load(g, MAX_GAMES); CHECK(n == 1 && strcmp(g[0].label, "After") == 0);

    /* cap at 64 */
    char *many = malloc(64 * 1024); many[0] = 0;
    for (int i = 0; i < 70; i++) strcat(many, LINE("G", ROM("Metroid.gba"), "", LAUNCH));
    wr("/Roms/surprise.json", many); free(many);
    CHECK(games_load(g, 1000) == 64);

    wr("/Roms/surprise.json", ""); CHECK(games_load(g, MAX_GAMES) == 0);
}

static void test_launch(void) {
    Game g = {0};
    strcpy(g.label, "Metroid"); strcpy(g.rompath, ROM("Metroid.gba")); strcpy(g.launch, LAUNCH);
    strcpy(g.imgpath, "/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Imgs/Metroid.png");
    char b[1200];
    launch_build_line(b, sizeof b, &g);
    CHECK(strcmp(b, "{\"label\":\"Metroid\",\"rompath\":\"" ROM("Metroid.gba") "\",\"imgpath\":\"/mnt/SDCARD/Emu/GBA/../../Roms/GBA/Imgs/Metroid.png\",\"launch\":\"" LAUNCH "\",\"type\":5}") == 0);
    g.imgpath[0] = 0;
    launch_build_line(b, sizeof b, &g);
    CHECK(strcmp(b, "{\"label\":\"Metroid\",\"rompath\":\"" ROM("Metroid.gba") "\",\"launch\":\"" LAUNCH "\",\"type\":5}") == 0);

    /* recents file missing; .showRecents absent -> hidden file */
    CHECK(launch_game(&g) == 0);
    const char *cmd = slurp("/.tmp_update/cmd_to_run.sh", 0);
    CHECK(cmd && strcmp(cmd, "LD_PRELOAD=/mnt/SDCARD/miyoo/app/../lib/libpadsp.so \"" LAUNCH "\" \"" ROM("Metroid.gba") "\"") == 0);
    char qs[700]; snprintf(qs, sizeof qs, "%s/quick_switch", tmpd);
    CHECK(access(qs, F_OK) == 0);
    const char *r = slurp("/Roms/recentlist-hidden.json", 0);
    CHECK(r && strcmp(r, "{\"label\":\"Metroid\",\"rompath\":\"" ROM("Metroid.gba") "\",\"launch\":\"" LAUNCH "\",\"type\":5}\n") == 0);
    CHECK(!exists("/Roms/recentlist.json"));
    CHECK(!exists("/Roms/recentlist-hidden.json.tmp"));

    /* dedupe + prepend, other lines preserved in order */
    wr("/Roms/recentlist-hidden.json",
       LINE("Old1", ROM("My Game.gba"), "", LAUNCH)
       LINE("MetroidOld", ROM("Metroid.gba"), "", LAUNCH)
       LINE("Old3", ROM("Zelda.gba"), "", LAUNCH));
    unlink(qs);
    CHECK(launch_game(&g) == 0);
    r = slurp("/Roms/recentlist-hidden.json", 0);
    char want[2048] = "";
    strcat(want, "{\"label\":\"Metroid\",\"rompath\":\"" ROM("Metroid.gba") "\",\"launch\":\"" LAUNCH "\",\"type\":5}\n");
    strcat(want, LINE("Old1", ROM("My Game.gba"), "", LAUNCH));
    strcat(want, LINE("Old3", ROM("Zelda.gba"), "", LAUNCH));
    CHECK(r && strcmp(r, want) == 0);
    CHECK(access(qs, F_OK) == 0);

    /* .showRecents toggles file */
    touchf("/.tmp_update/config/.showRecents");
    CHECK(launch_game(&g) == 0);
    r = slurp("/Roms/recentlist.json", 0);
    CHECK(r && strncmp(r, "{\"label\":\"Metroid\"", 18) == 0);

    /* spaces in paths */
    Game s = {0}; strcpy(s.label, "Spaced"); strcpy(s.rompath, ROM("My Game.gba")); strcpy(s.launch, LAUNCH);
    CHECK(launch_game(&s) == 0);
    cmd = slurp("/.tmp_update/cmd_to_run.sh", 0);
    CHECK(cmd && strcmp(cmd, "LD_PRELOAD=/mnt/SDCARD/miyoo/app/../lib/libpadsp.so \"" LAUNCH "\" \"" ROM("My Game.gba") "\"") == 0);
}

int main(void) {
    setup();
    test_paths(); test_games(); test_launch();
    printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
