#include "GameEngine.h"
#include "LCD.h"
#include "Buzzer.h"
#include "Music.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SCREEN_W 240
#define SCREEN_H 240

// loading screen tips
typedef struct { int is_fb; const char *line1; const char *line2; } Tip;
static const Tip g_tips[8] = {
    {1, "Water is my", "weakness! Avoid it!"},
    {0, "Lava will burn", "me! Stay clear!"},
    {1, "We must BOTH", "reach our doors!"},
    {0, "Collect gems","for bonus points!"},
    {1, "Moving platforms","help us climb up!"},
    {0, "Press buttons","to open new paths!"},
    {1, "Jump across", "gaps carefully!"},
    {0, "Teamwork wins", "every time!"},
};

// level select screen
static char * g_lvl_names[5] = {
    "Crystal Caves",
    "Forest Temple",
    "Goblin Lair",
    "Volcano's Heart",
    "The Final Run",
};
#define LEVELS_AVAILABLE 5

// check joystick direction
static bool dir_up(int d)    { return d == N || d == NW || d == NE; }
static bool dir_down(int d)  { return d == S || d == SW || d == SE; }
static bool dir_left(int d)  { return d == W || d == SW || d == NW; }
static bool dir_right(int d) { return d == E || d == SE || d == NE; }

// handles joystick debouncing
static bool just_pressed(GameEngine *ge, int cur_dir) {
    bool moved = false;
    if (cur_dir != CENTRE && ge->prev_dir == CENTRE)
        moved = true;
    ge->prev_dir = cur_dir;
    return moved;
}

// cave background
static void draw_cave_bg(void) {
    LCD_Draw_Rect(0, 0, SCREEN_W, SCREEN_H, 9, 1);
    for (int wy = 18; wy < SCREEN_H; wy += 30) {
        LCD_Draw_Rect(0, wy,     SCREEN_W, 2, 12, 1);
        LCD_Draw_Rect(0, wy + 1, SCREEN_W, 1,  0, 1);
    }
    for (int br = 0; br < 9; br++) {
        int xoff;
        if (br % 2 == 0) xoff = 0;
        else xoff = 30;
        int ytop = br * 30;
        int ybot = ytop + 27;
        if (ybot > SCREEN_H - 1) ybot = SCREEN_H - 1;
        for (int bx = xoff; bx < SCREEN_W; bx += 60)
            LCD_Draw_Rect(bx, ytop, 1, ybot - ytop, 0, 1);
    }
}

// settings volume bar
static void draw_volume_bar(int vol, int x, int y) {
    LCD_Draw_Rect(x, y, 142, 10, 0, 0);
    int filled = vol / 10;
    for (int s = 0; s < 10; s++) {
        int col;
        if (s < filled) col = 3;
        else col = 13;
        LCD_Draw_Rect(x + 1 + s * 14, y + 1, 13, 8, col, 1);
    }
}

//switches game to new screen
static void enter_state(GameEngine *ge, GameState new_state) {
    ge->state          = new_state;
    ge->state_enter_ms = HAL_GetTick();
}

static uint32_t ms_in_state(GameEngine *ge) {
    return HAL_GetTick() - ge->state_enter_ms;
}

// places 4 diamonds randomly on platforms each time a level loads
static void place_random_diamonds(Level *lvl) {
    lvl->diamond_count = 0;
    int attempts = 0;

    while (lvl->diamond_count < 4 && attempts < 60) {
        attempts++;

        int pi = rand() % lvl->platform_count;
        Platform *pl = &lvl->platforms[pi];
        if (!pl->active || pl->w < 16) continue;

        int dx = pl->x + 4 + rand() % (pl->w - 12);
        int dy = pl->y - 8;

        if (dy < 5 || dx < 2 || dx > 228) continue;

        // skip if too close to an exit door
        Rect *ef = &lvl->exit_fire;
        Rect *ew = &lvl->exit_water;
        if (dx < ef->x + ef->w + 6 && dx + 8 > ef->x - 6 &&
            dy < ef->y + ef->h + 6 && dy + 8 > ef->y - 6) continue;
        if (dx < ew->x + ew->w + 6 && dx + 8 > ew->x - 6 &&
            dy < ew->y + ew->h + 6 && dy + 8 > ew->y - 6) continue;

        // skip if on top of a hazard
        bool bad = false;
        for (int h = 0; h < lvl->hazard_count; h++) {
            Hazard *hz = &lvl->hazards[h];
            if (dx < hz->x + hz->w && dx + 8 > hz->x &&
                dy < hz->y + hz->h && dy + 8 > hz->y) {
                bad = true;
                break;
            }
        }
        if (bad) continue;

        // skip if too close to another diamond
        for (int d = 0; d < lvl->diamond_count; d++) {
            int ddx = dx - lvl->diamonds[d].x;
            int ddy = dy - lvl->diamonds[d].y;
            if (ddx < 0) ddx = -ddx;
            if (ddy < 0) ddy = -ddy;
            if (ddx < 16 && ddy < 16) {
                bad = true;
                break;
            }
        }
        if (bad) continue;

        lvl->diamonds[lvl->diamond_count].x = dx;
        lvl->diamonds[lvl->diamond_count].y = dy;
        lvl->diamonds[lvl->diamond_count].collected = false;
        lvl->diamond_count++;
    }
}

static void load_level(GameEngine *ge, int idx) {
    ge->level_idx = idx;
    ge->crumble_timer = CRUMBLE_TICKS;
    ge->fb_diamonds = 0;
    ge->wg_diamonds = 0;
    ge->level_start_ms = HAL_GetTick();
    Level_Load(&ge->level, idx);
    place_random_diamonds(&ge->level);
    Player_Init(&ge->fireboy, PLAYER_FIRE, ge->level.fire_spawn_x, ge->level.fire_spawn_y);
    Player_Init(&ge->watergirl, PLAYER_WATER, ge->level.water_spawn_x, ge->level.water_spawn_y);
}

static bool overlaps_rect(int px, int py, Rect *r) {
    bool x_overlap = px < r->x + r->w && px + PLAYER_W > r->x;
    bool y_overlap = py < r->y + r->h && py + PLAYER_HEIGHT >= r->y;
    return x_overlap && y_overlap;
}

static void check_hazards(GameEngine *ge) {
    Level *lvl = &ge->level;
    bool dead = false;

    for (int i = 0; i < lvl->hazard_count; i++) {
        Hazard *h = &lvl->hazards[i];
        // shrink the kill box slightly so near-misses don't kill
        int hazard_w = h->w > 4 ? h->w - 4 : h->w;
        Rect hr = {h->x + 2, h->y, hazard_w, h->h};
        if (h->type == HAZARD_WATER && overlaps_rect(ge->fireboy.x, ge->fireboy.y, &hr))
            dead = true;
        if (h->type == HAZARD_LAVA && overlaps_rect(ge->watergirl.x, ge->watergirl.y, &hr))
            dead = true;
    }

    // check geysers
    for (int i = 0; i < lvl->platform_count; i++) {
        Platform *p = &lvl->platforms[i];
        if (!p->active || p->type != PLAT_GEYSER) continue;
        int col_h = p->move_max - p->y;
        if (col_h <= 4) continue;
        int geyser_w = p->w > 2 ? p->w - 2 : p->w;
        Rect gr = {p->x + 1, p->y, geyser_w, col_h};
        if (overlaps_rect(ge->fireboy.x, ge->fireboy.y, &gr)) dead = true;
        if (overlaps_rect(ge->watergirl.x, ge->watergirl.y, &gr)) dead = true;
    }

    if (ge->fireboy.y > SCREEN_H) dead = true;
    if (ge->watergirl.y > SCREEN_H) dead = true;

    if (dead) {
        enter_state(ge, GS_DEAD);
    }
}

static void check_goblins(GameEngine *ge) {
    for (int i = 0; i < ge->level.goblin_count; i++) {
        Goblin *g = &ge->level.goblins[i];
        if (!g->active) continue;

        bool fb_hit = ge->fireboy.x < g->x + GOBLIN_W &&
                      ge->fireboy.x + PLAYER_W > g->x &&
                      ge->fireboy.y < g->y + GOBLIN_H &&
                      ge->fireboy.y + PLAYER_HEIGHT > g->y;

        bool wg_hit = ge->watergirl.x < g->x + GOBLIN_W &&
                      ge->watergirl.x + PLAYER_W > g->x &&
                      ge->watergirl.y < g->y + GOBLIN_H &&
                      ge->watergirl.y + PLAYER_HEIGHT > g->y;

        if (fb_hit || wg_hit) {
            enter_state(ge, GS_DEAD);
            return;
        }
    }
}

static void check_win(GameEngine *ge) {
    bool fb_at = overlaps_rect(ge->fireboy.x, ge->fireboy.y, &ge->level.exit_fire);
    bool wg_at = overlaps_rect(ge->watergirl.x, ge->watergirl.y, &ge->level.exit_water);
    if (fb_at && wg_at) {
        enter_state(ge, GS_WIN);
    }
}

static void draw_hud(GameEngine *ge) {
    char buf[24];

    // background
    LCD_Draw_Rect(0, 0, SCREEN_W, TOP_BAR_H, 9, 1);
    LCD_Draw_Rect(0, 0, 75, TOP_BAR_H, 0, 1);
    LCD_Draw_Rect(165, 0, 75, TOP_BAR_H, 0, 1);

    // gold border lines and dividers
    LCD_Draw_Rect(0,  0, SCREEN_W, 1, 10, 1);
    LCD_Draw_Rect(0, 19, SCREEN_W, 1, 10, 1);
    LCD_Draw_Rect(74,  0, 2, TOP_BAR_H, 10, 1);
    LCD_Draw_Rect(164, 0, 2, TOP_BAR_H, 10, 1);

    // colour strips under border
    LCD_Draw_Rect(1,   1, 72, 2, 2, 1);
    LCD_Draw_Rect(167, 1, 72, 2, 4, 1);

    // fireboy diamond count
    LCD_printString("FB", 3, 4, 2, 1);
    LCD_Draw_Rect(22, 7, 5, 5, 6, 1);
    LCD_Draw_Rect(23, 8, 3, 3, 10, 1);
    snprintf(buf, sizeof(buf), "%d", ge->fb_diamonds);
    LCD_printString(buf, 29, 7, 1, 1);

    // watergirl diamond count
    snprintf(buf, sizeof(buf), "%d", ge->wg_diamonds);
    LCD_printString(buf, 169, 7, 1, 1);
    LCD_Draw_Rect(178, 7, 5, 5, 14, 1);
    LCD_Draw_Rect(179, 8, 3, 3, 4, 1);
    LCD_printString("WG", 185, 4, 4, 1);

    // centre: timer or countdown
    if (ge->level.crumbles && ge->state == GS_PLAY) {
        uint32_t elapsed_ms = HAL_GetTick() - ge->level_start_ms;
        int secs;
        if (elapsed_ms >= 60000u) secs = 0;
        else secs = (int)((60000u - elapsed_ms) / 1000u);
        snprintf(buf, sizeof(buf), "TIME:%02d", secs);
        int timer_col;
        if (secs <= 10) timer_col = 2;
        else timer_col = 5;
        LCD_printString(buf, 82, 6, timer_col, 1);
    } else {
        char *n = g_lvl_names[ge->level_idx < 5u ? ge->level_idx : 0u];
        int nl = 0;
        while (n[nl]) nl++;
        int nx = 76 + (90 - nl * 6) / 2;
        if (nx < 76) nx = 76;
        LCD_printString(n, (uint16_t)nx, 2, 10, 1);

        uint32_t es = (HAL_GetTick() - ge->level_start_ms) / 1000u;
        if (es > 5999u) es = 5999u;
        snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)(es / 60u), (unsigned)(es % 60u));
        LCD_printString(buf, 101, 11, 13, 1);
    }
}

void GameEngine_Init(GameEngine *ge) {
    memset(ge, 0, sizeof(GameEngine));
    srand(HAL_GetTick());
    ge->state = GS_TITLE;
    ge->volume = 80;
    ge->cursor_pos = 0;
    ge->sel_level = 0;
    ge->speech_idx = 0;
    Player_Init(&ge->fireboy, PLAYER_FIRE, 40, 55);
    Player_Init(&ge->watergirl, PLAYER_WATER, 180, 55);
    Music_Init();
}

void GameEngine_Update(GameEngine *ge, UserInput fb_in, UserInput wg_in, bool pause_btn) {
    int d;
    if (fb_in.direction != CENTRE) d = fb_in.direction;
    else d = wg_in.direction;
    bool pressed = just_pressed(ge, d);

    switch (ge->state) {

    case GS_TITLE:
        if (pressed) {
            if (dir_up(d)) {
                if (ge->cursor_pos > 0) ge->cursor_pos--;
            } else if (dir_down(d)) {
                if (ge->cursor_pos < 2) ge->cursor_pos++;
            } else if (dir_right(d)) {
                if (ge->cursor_pos == 0) {
                    ge->cursor_pos = 0;
                    enter_state(ge, GS_LEVEL_SELECT);
                } else if (ge->cursor_pos == 1) {
                    ge->cursor_pos = 0;
                    enter_state(ge, GS_SETTINGS);
                } else {
                    ge->about_scroll = 0;
                    ge->cursor_pos = 0;
                    enter_state(ge, GS_ABOUT);
                }
            }
        }
        break;

    case GS_LEVEL_SELECT:
        if (pressed) {
            if (dir_up(d)) {
                if (ge->cursor_pos > 0) ge->cursor_pos--;
            } else if (dir_down(d)) {
                if (ge->cursor_pos < 5) ge->cursor_pos++;
            } else if (dir_left(d)) {
                ge->cursor_pos = 0;
                enter_state(ge, GS_TITLE);
            } else if (dir_right(d) || d == N || d == S) {
                if (ge->cursor_pos == 5) {
                    ge->cursor_pos = 0;
                    enter_state(ge, GS_TITLE);
                } else if (ge->cursor_pos < LEVELS_AVAILABLE) {
                    ge->sel_level = ge->cursor_pos;
                    ge->loading_ticks = 0;
                    ge->speech_idx = (ge->speech_idx + 1) % 8;
                    enter_state(ge, GS_LOADING);
                }
            }
        }
        break;

    case GS_SETTINGS:
        if (pressed) {
            if (dir_left(d)) {
                if (ge->volume >= 10) ge->volume -= 10;
            } else if (dir_right(d)) {
                if (ge->volume <= 90) ge->volume += 10;
            } else if (d == W || d == SW || d == NW || d == S) {
                ge->cursor_pos = 1;
                enter_state(ge, GS_TITLE);
            }
        }
        break;

    case GS_LOADING:
        if ((HAL_GetTick() - ge->state_enter_ms) >= 8000u) {
            load_level(ge, ge->sel_level);
            if (ge->sel_level == 3 || ge->sel_level == 4) {
                ge->story_page = 0;
                enter_state(ge, GS_STORY);
            } else {
                enter_state(ge, GS_PLAY);
            }
        }
        break;

    case GS_STORY:
        if (ms_in_state(ge) >= STORY_PAGE_MS || pressed) {
            ge->story_page++;
            if (ge->story_page >= STORY_PAGES) {
                enter_state(ge, GS_PLAY);
            } else {
                enter_state(ge, GS_STORY);
            }
        }
        break;

    case GS_PLAY:
        if (pause_btn) {
            ge->cursor_pos = 0;
            enter_state(ge, GS_PAUSE);
            break;
        }
        Level_Update(&ge->level);
        Level_CheckButtons(&ge->level, ge->fireboy.x, ge->fireboy.y, ge->watergirl.x, ge->watergirl.y);

        for (int i = 0; i < ge->level.diamond_count; i++) {
            Diamond *d = &ge->level.diamonds[i];
            if (d->collected) continue;
            Rect dr = {d->x, d->y, 8, 8};
            if (overlaps_rect(ge->fireboy.x, ge->fireboy.y, &dr)) {
                d->collected = true;
                ge->fb_diamonds++;
            } else if (overlaps_rect(ge->watergirl.x, ge->watergirl.y, &dr)) {
                d->collected = true;
                ge->wg_diamonds++;
            }
        }

        Player_Update(&ge->fireboy, fb_in, &ge->level);
        // watergirl is frozen in level 3 (goblin lair rescue mechanic)
        if (ge->level_idx != 2)
            Player_Update(&ge->watergirl, wg_in, &ge->level);

        if (ge->level.crumbles) {
            uint32_t elapsed_ms = HAL_GetTick() - ge->level_start_ms;
            if (elapsed_ms >= 60000u) {
                enter_state(ge, GS_DEAD);
                break;
            }
        }

        check_hazards(ge);
        check_goblins(ge);
        check_win(ge);
        break;

    case GS_PAUSE:
        if (pause_btn || dir_right(d)) {
            enter_state(ge, GS_PLAY);
        } else if (pressed && dir_left(d)) {
            ge->cursor_pos = 0;
            enter_state(ge, GS_TITLE);
        } else if (pressed && (dir_down(d) || dir_up(d))) {
            if (ge->cursor_pos == 0) ge->cursor_pos = 1;
            else ge->cursor_pos = 0;
        }
        break;

    case GS_DEAD:
        if (ms_in_state(ge) > DEAD_PAUSE_MS) {
            ge->cursor_pos = 0;
            enter_state(ge, GS_DEAD_MENU);
        }
        break;

    case GS_DEAD_MENU:
        if (pressed) {
            if (dir_up(d)) {
                if (ge->cursor_pos > 0) ge->cursor_pos--;
            } else if (dir_down(d)) {
                if (ge->cursor_pos < 2) ge->cursor_pos++;
            } else if (dir_right(d)) {
                if (ge->cursor_pos == 0) {
                    load_level(ge, ge->level_idx);
                    enter_state(ge, GS_PLAY);
                } else if (ge->cursor_pos == 1) {
                    ge->cursor_pos = 0;
                    enter_state(ge, GS_LEVEL_SELECT);
                } else {
                    ge->cursor_pos = 0;
                    enter_state(ge, GS_TITLE);
                }
            }
        }
        break;

    case GS_WIN:
        if (ms_in_state(ge) > WIN_PAUSE_MS) {
            int next = ge->level_idx + 1;
            if (next >= NUM_LEVELS) {
                ge->cursor_pos = 0;
                enter_state(ge, GS_LEVEL_SELECT);
            } else {
                ge->sel_level     = next;
                ge->loading_ticks = 0;
                ge->speech_idx    = (ge->speech_idx + 1) % 8;
                enter_state(ge, GS_LOADING);
            }
        }
        break
    case GS_ABOUT:
        if (dir_down(d)) {
            ge->about_scroll += 8;
            if (ge->about_scroll > 480) ge->about_scroll = 480;
        } else if (dir_up(d)) {
            ge->about_scroll -= 8;
            if (ge->about_scroll < 0) ge->about_scroll = 0;
        } else if (pressed && dir_left(d)) {
            ge->cursor_pos = 2;
            enter_state(ge, GS_TITLE);
        }
        break;

    default: break;
    }

    MusicTrack mtrack;
    switch (ge->state) {
        case GS_PLAY:      mtrack = MTRACK_PLAY;  break;
        case GS_PAUSE:     mtrack = MTRACK_PLAY;  break;
        case GS_DEAD:      mtrack = MTRACK_DEAD;  break;
        case GS_DEAD_MENU: mtrack = MTRACK_TITLE; break;
        case GS_WIN:       mtrack = MTRACK_WIN;   break;
        default:           mtrack = MTRACK_TITLE; break;
    }
    Music_Update(mtrack, ge->volume);
}

void GameEngine_DrawScene(GameEngine *ge) {
    char buf[32];

    switch (ge->state) {

    case GS_TITLE: {
        draw_cave_bg();

        // title bar
        LCD_Draw_Rect(0,  0, 240, 20,  0, 1);
        LCD_Draw_Rect(0, 19, 240,  2, 10, 1);
        LCD_printString("FIREBOY & ",  44, 4, 2, 1);
        LCD_printString("WATERGIRL",  124, 4, 4, 1);

        // character panels
        LCD_Draw_Rect(119, 22, 2, 86, 10, 1);
        LCD_printString("FIREBOY",    22, 26, 2, 1);
        LCD_printString("WATERGIRL", 154, 26, 4, 1);

        // fire pedestal under fireboy sprite
        LCD_Draw_Rect(30, 105, 42, 6, 2, 1);
        LCD_Draw_Rect(30, 105, 42, 1, 6, 1);

        // water pedestal under watergirl sprite
        LCD_Draw_Rect(170, 105, 42, 6, 4, 1);
        LCD_Draw_Rect(170, 105, 42, 1, 1, 1);

        // menu box
        LCD_Draw_Rect(58, 120, 124, 72, 10, 0);
        LCD_Draw_Rect(60, 122, 120, 68,  0, 0);
        LCD_Draw_Rect(62, 124, 116, 64, 12, 1);

        char *items[3] = {"PLAY", "SETTINGS", "ABOUT"};
        for (int i = 0; i < 3; i++) {
            int iy = 130 + i * 20;
            if (ge->cursor_pos == i) {
                LCD_printString(">",      70, iy,  6, 1);
                LCD_printString(items[i], 84, iy,  1, 1);
            } else {
                LCD_printString(" ",      70, iy,  1, 1);
                LCD_printString(items[i], 84, iy, 13, 1);
            }
        }

        // cycling speech bubble at the bottom
        int t_idx = (int)((ms_in_state(ge) / 3000u) % 8u);
        const Tip *tp = &g_tips[t_idx];
        LCD_Draw_Rect( 8, 196, 224, 40, 10, 0);
        LCD_Draw_Rect(10, 198, 220, 36,  0, 0);
        LCD_Draw_Rect(11, 199, 218, 34,  9, 1);
        int sc;
        if (tp->is_fb) sc = 2;
        else sc = 4;
        LCD_printString(tp->is_fb ? "FIREBOY:" : "WATERGIRL:", 16, 203, sc, 1);
        LCD_printString(tp->line1, 16, 213,  1, 1);
        LCD_printString(tp->line2, 16, 223, 13, 1);
        break;
    }

    case GS_LEVEL_SELECT: {
        draw_cave_bg();
        LCD_printString("SELECT LEVEL", 50, 10, 10, 1);
        LCD_Draw_Rect(0, 22, 240, 1, 12, 1);

        for (int i = 0; i < 5; i++) {
            int iy = 32 + i * 30;
            bool avail = (i < LEVELS_AVAILABLE);
            int col;
            if (avail) col = 1;
            else col = 13;

            if (ge->cursor_pos == i && avail) {
                LCD_Draw_Rect(4, iy - 2, 232, 18, 12, 1);
                LCD_printString(">", 6, iy, 6, 1);
                col = 10;
            }

            snprintf(buf, sizeof(buf), "%d. %s", i + 1, g_lvl_names[i]);
            LCD_printString(buf, 18, iy, col, 1);
            if (i == 4)
                LCD_printString("FINAL", 184, iy, 2, 1);
            else if (!avail)
                LCD_printString("(soon)", 168, iy, 13, 1);
        }

        // back option at the bottom
        int by = 32 + 5 * 30;
        if (ge->cursor_pos == 5) {
            LCD_Draw_Rect(4, by - 2, 232, 18, 12, 1);
            LCD_printString("> BACK", 6, by, 6, 1);
        } else {
            LCD_printString("  BACK", 6, by, 13, 1);
        }
        LCD_printString("UP/DN=move  E=select  W=back", 4, 226, 13, 1);
        break;
    }

    case GS_SETTINGS: {
        draw_cave_bg();
        LCD_printString("SETTINGS", 75, 10, 10, 1);
        LCD_Draw_Rect(0, 22, 240, 1, 12, 1);

        LCD_printString("VOLUME", 20, 70, 1, 1);
        snprintf(buf, sizeof(buf), "%3d%%", ge->volume);
        LCD_printString(buf, 188, 70, 10, 1);
        draw_volume_bar(ge->volume, 20, 85);

        LCD_printString("< W  decrease", 20, 110, 13, 1);
        LCD_printString("increase  E >", 120, 110, 13, 1);

        LCD_Draw_Rect(20, 180, 80, 20, 12, 1);
        LCD_printString("< BACK", 28, 185, 1, 1);
        LCD_printString("Push W or S to go back", 10, 218, 13, 1);
        break;
    }

    case GS_LOADING: {
        draw_cave_bg();

        LCD_Draw_Rect(20, 8, 200, 18, 12, 1);
        snprintf(buf, sizeof(buf), "Loading: %s", g_lvl_names[ge->sel_level]);
        LCD_printString(buf, 24, 12, 10, 1);

        // speech bubble
        LCD_Draw_Rect(10, 80, 220, 90, 0, 0);
        LCD_Draw_Rect(11, 81, 218, 88, 9, 1);

        const Tip *tp = &g_tips[ge->speech_idx % 8];
        int sc;
        if (tp->is_fb) sc = 2;
        else sc = 4;
        LCD_printString(tp->is_fb ? "FIREBOY says:" : "WATERGIRL says:", 18, 88, sc, 1);
        LCD_printString(tp->line1, 18, 108, 1, 1);
        LCD_printString(tp->line2, 18, 122, 1, 1);

        // progress bar
        uint32_t _elapsed = HAL_GetTick() - ge->state_enter_ms;
        if (_elapsed > 8000u) _elapsed = 8000u;
        int filled_w = (int)((_elapsed * 200u) / 8000u);
        LCD_Draw_Rect(20, 195, 200, 12, 0, 0);
        LCD_Draw_Rect(21, 196, filled_w, 10, 3, 1);
        LCD_printString("Please wait...", 68, 214, 13, 1);
        break;
    }

    case GS_PLAY:
        Level_Draw(&ge->level);
        draw_hud(ge);

        // tutorial hints for level 1 (first 12 seconds)
        if (ge->level_idx == 0) {
            uint32_t t = ms_in_state(ge);
            if (t < 12000u) {
                LCD_Draw_Rect(74,  197, 57, 18, 0, 1);
                LCD_printString("LAVA!", 80, 200, 5, 1);
                LCD_printString("->kills WG", 80, 209, 5, 1);

                LCD_Draw_Rect(139, 197, 57, 18, 0, 1);
                LCD_printString("WATER!", 143, 200, 4, 1);
                LCD_printString("->kills FB", 143, 209, 4, 1);
            }
            if (t > 4000u && t < 15000u) {
                LCD_Draw_Rect(30, 50, 180, 12, 0, 1);
                LCD_printString("Both reach your EXIT doors!", 32, 52, 10, 1);
            }
        }

        // level 3 goblin lair — show chain and rescue hints
        if (ge->level_idx == 2) {
            uint32_t t = ms_in_state(ge);
            LCD_Draw_Rect(214, 20, 2, 8, 12, 1);
            LCD_Draw_Rect(230, 20, 2, 8, 12, 1);
            if (t < 8000u) {
                LCD_Draw_Rect(128, 22, 84, 10, 0, 1);
                LCD_printString("SAVE ME!", 130, 24, 4, 1);
            }
            if (t < 12000u) {
                LCD_Draw_Rect(0, 66, 240, 20, 0, 1);
                int hint = (int)((t / 4000u) % 3u);
                if (hint == 0) {
                    LCD_printString("Only Fireboy moves!", 10, 68, 6, 1);
                    LCD_printString("Reach Watergirl to rescue!", 2, 78, 1, 1);
                } else if (hint == 1) {
                    LCD_printString("Find the lever — it builds", 4, 68, 10, 1);
                    LCD_printString("a bridge over the lava!", 10, 78, 10, 1);
                } else {
                    LCD_printString("Dodge the goblin AND", 10, 68, 2, 1);
                    LCD_printString("the rolling boulder!", 10, 78, 2, 1);
                }
            }
        }

        // level 5 final run hints
        if (ge->level_idx == 4) {
            uint32_t t = ms_in_state(ge);
            if (t < 8000u) {
                LCD_Draw_Rect(0, 50, 240, 20, 0, 1);
                int hint = (int)((t / 4000u) % 2u);
                if (hint == 0) {
                    LCD_printString("Ride the moving platform!", 4, 52, 6, 1);
                    LCD_printString("Pull lever -> opens bridge!", 2, 62, 10, 1);
                } else {
                    LCD_printString("Both reach your EXIT doors!", 2, 52, 1, 1);
                    LCD_printString("Time is running out — RUN!", 2, 62, 2, 1);
                }
            }
        }
        break;

    case GS_STORY: {
        // border flashes every 500ms
        int blink = (int)((ms_in_state(ge) / 500u) % 2u);
        int border_col;
        if (blink) border_col = 2;
        else border_col = 5;
        LCD_Draw_Rect(0,   0, 240,   4, border_col, 1);
        LCD_Draw_Rect(0, 236, 240,   4, border_col, 1);
        LCD_Draw_Rect(0,   0,   4, 240, border_col, 1);
        LCD_Draw_Rect(236, 0,  4, 240, border_col, 1);

        draw_cave_bg();

        // level 4 story pages
        if (ge->level_idx == 3) {
            if (ge->story_page == 0) {
                LCD_Draw_Rect(10, 10, 220, 18, 0, 1);
                LCD_printString("VOLCANO'S HEART", 26, 14, 2, 1);

                LCD_Draw_Rect(8, 38, 224, 100, 0, 1);
                LCD_Draw_Rect(9, 39, 222,  98, 9, 1);
                LCD_printString("Deep beneath the earth,", 14, 46, 1, 1);
                LCD_printString("an ancient volcano stirs.", 14, 58, 1, 1);
                LCD_printString("Lava geysers roar to life.", 14, 70, 1, 1);
                LCD_printString("The Crystal Shard waits", 14, 82, 1, 1);
                LCD_printString("at the very summit...", 14, 94, 1, 1);

                LCD_Draw_Line(40,  210, 90,  155, 5);
                LCD_Draw_Line(90,  155, 110, 140, 6);
                LCD_Draw_Line(110, 140, 130, 155, 6);
                LCD_Draw_Line(130, 155, 180, 210, 5);
                LCD_Draw_Line(95,  158, 85,  200, 2);
                LCD_Draw_Line(125, 158, 135, 200, 2);

                LCD_printString("Press any button...", 28, 222, 13, 1);

            } else if (ge->story_page == 1) {
                LCD_Draw_Rect(10, 10, 220, 18, 12, 1);
                LCD_printString("FIREBOY & WATERGIRL", 22, 14, 10, 1);

                LCD_Draw_Rect(8, 38, 145, 60, 0, 1);
                LCD_Draw_Rect(9, 39, 143, 58, 9, 1);
                LCD_printString("FIREBOY:", 14, 44, 2, 1);
                LCD_printString("I can feel the", 14, 56, 1, 1);
                LCD_printString("heat! Watch the", 14, 68, 1, 1);
                LCD_printString("geysers - they", 14, 80, 1, 1);
                LCD_printString("will kill us both!", 14, 92, 1, 1);

                LCD_Draw_Rect(8, 108, 145, 60, 0, 1);
                LCD_Draw_Rect(9, 109, 143, 58, 9, 1);
                LCD_printString("WATERGIRL:", 14, 114, 4, 1);
                LCD_printString("Stay together.", 14, 126, 1, 1);
                LCD_printString("One wrong step", 14, 138, 1, 1);
                LCD_printString("and the volcano", 14, 150, 1, 1);
                LCD_printString("wins...", 14, 162, 1, 1);

                LCD_Draw_Rect(8, 178, 224, 36, 5, 1);
                LCD_Draw_Rect(9, 179, 222, 34, 0, 1);
                LCD_printString("GEYSERS KILL EVERYONE.", 18, 184, 2, 1);
                LCD_printString("Time your crossings.", 22, 196, 1, 1);

                LCD_printString("Press any button...", 28, 222, 13, 1);

            } else {
                LCD_Draw_Rect(15, 60, 210, 3, 5, 1);
                LCD_Draw_Rect(15, 64, 210, 3, 2, 1);
                LCD_Draw_Rect(15, 68, 210, 3, 5, 1);

                LCD_printString("THE", 90, 80, 5, 2);
                LCD_printString("VOLCANO'S", 50, 100, 2, 2);
                LCD_printString("HEART", 74, 120, 5, 2);

                LCD_Draw_Rect(15, 140, 210, 3, 5, 1);
                LCD_Draw_Rect(15, 144, 210, 3, 2, 1);
                LCD_Draw_Rect(15, 148, 210, 3, 5, 1);

                LCD_Draw_Rect(20, 162, 200, 26, 0, 1);
                LCD_printString("Lava geysers ERUPT!", 22, 166, 2, 1);
                LCD_printString("Reach the summit!", 30, 178, 1, 1);

                if (blink)
                    LCD_printString(">>> INTO THE VOLCANO <<<", 10, 210, 2, 1);
                else
                    LCD_printString(">>> INTO THE VOLCANO <<<", 10, 210, 5, 1);
            }
            break;
        }

        // level 5 story pages
        if (ge->story_page == 0) {
            LCD_Draw_Rect(10, 10, 220, 18, 0, 1);
            LCD_printString("THE FINAL RUN", 38, 14, 2, 1);

            LCD_Draw_Rect(8, 38, 224, 100, 0, 1);
            LCD_Draw_Rect(9, 39, 222,  98, 9, 1);
            LCD_printString("The ancient temple is", 14, 46, 1, 1);
            LCD_printString("collapsing. A dark seal", 14, 58, 1, 1);
            LCD_printString("has been broken and the", 14, 70, 1, 1);
            LCD_printString("Goblin King's curse", 14, 82, 1, 1);
            LCD_printString("spreads through every", 14, 94, 1, 1);
            LCD_printString("stone and shadow...", 14, 106, 1, 1);

            LCD_Draw_Line(30,  150, 50,  180, 12);
            LCD_Draw_Line(50,  180, 40,  220, 12);
            LCD_Draw_Line(170, 145, 200, 175, 12);
            LCD_Draw_Line(200, 175, 185, 215,  0);
            LCD_Draw_Line(100, 155, 120, 195, 12);

            LCD_printString("Press any button...", 28, 222, 13, 1);

        } else if (ge->story_page == 1) {
            LCD_Draw_Rect(10, 10, 220, 18, 12, 1);
            LCD_printString("FIREBOY & WATERGIRL", 22, 14, 10, 1);

            LCD_Draw_Rect(8, 38, 145, 60, 0, 1);
            LCD_Draw_Rect(9, 39, 143, 58, 9, 1);
            LCD_printString("FIREBOY:", 14, 44, 2, 1);
            LCD_printString("We have to reach", 14, 56, 1, 1);
            LCD_printString("the sacred exits", 14, 68, 1, 1);
            LCD_printString("before it all", 14, 80, 1, 1);
            LCD_printString("comes down!", 14, 92, 1, 1);

            LCD_Draw_Rect(8, 108, 145, 60, 0, 1);
            LCD_Draw_Rect(9, 109, 143, 58, 9, 1);
            LCD_printString("WATERGIRL:", 14, 114, 4, 1);
            LCD_printString("Together! No", 14, 126, 1, 1);
            LCD_printString("matter what the", 14, 138, 1, 1);
            LCD_printString("goblins throw", 14, 150, 1, 1);
            LCD_printString("at us!", 14, 162, 1, 1);

            LCD_Draw_Rect(8, 178, 224, 36, 2, 1);
            LCD_Draw_Rect(9, 179, 222, 34, 0, 1);
            LCD_printString("60 SECONDS.", 60, 184, 2, 1);
            LCD_printString("Survive or be buried.", 18, 196, 1, 1);

            LCD_printString("Press any button...", 28, 222, 13, 1);

        } else {
            LCD_Draw_Rect(15, 60, 210, 3, 5, 1);
            LCD_Draw_Rect(15, 64, 210, 3, 2, 1);
            LCD_Draw_Rect(15, 68, 210, 3, 5, 1);

            LCD_printString("THE", 90, 80, 5, 2);
            LCD_printString("FINAL", 62, 100, 2, 2);
            LCD_printString("RUN", 90, 120, 5, 2);

            LCD_Draw_Rect(15, 140, 210, 3, 5, 1);
            LCD_Draw_Rect(15, 144, 210, 3, 2, 1);
            LCD_Draw_Rect(15, 148, 210, 3, 5, 1);

            LCD_Draw_Rect(20, 162, 200, 26, 0, 1);
            LCD_printString("Temple collapses in 60s!", 14, 166, 6, 1);
            LCD_printString("Both players must escape!", 14, 178, 1, 1);

            if (blink)
                LCD_printString(">>> BRACE YOURSELF <<<", 14, 210, 2, 1);
            else
                LCD_printString(">>> BRACE YOURSELF <<<", 14, 210, 5, 1);
        }
        break;
    }

    case GS_PAUSE: {
        Level_Draw(&ge->level);
        draw_hud(ge);

        LCD_Draw_Rect(40, 70, 160, 110, 0, 1);
        LCD_Draw_Rect(42, 72, 156, 106, 9, 1);
        LCD_Draw_Rect(40, 70, 160,   2, 10, 1);
        LCD_Draw_Rect(40, 178, 160,  2, 10, 1);

        LCD_printString("PAUSED", 82, 82, 10, 1);
        LCD_Draw_Rect(42, 94, 156, 1, 12, 1);

        char *items[2] = {"RESUME", "QUIT TO MENU"};
        for (int i = 0; i < 2; i++) {
            int iy = 104 + i * 26;
            if (ge->cursor_pos == i) {
                LCD_Draw_Rect(44, iy - 2, 152, 16, 12, 1);
                LCD_printString(">", 50, iy, 6, 1);
                LCD_printString(items[i], 64, iy, 1, 1);
            } else {
                LCD_printString(" ", 50, iy, 1, 1);
                LCD_printString(items[i], 64, iy, 13, 1);
            }
        }

        LCD_printString("SW=confirm  W=quit", 46, 162, 13, 1);
        break;
    }

    case GS_DEAD:
        draw_cave_bg();
        if (ge->level_idx == 4) {
            LCD_printString("BURIED ALIVE!", 28, 80, 2, 2);
            LCD_printString("The temple claimed you.", 10, 120, 1, 1);
        } else {
            LCD_printString("YOU DIED!", 48, 90, 2, 2);
        }
        break;

    case GS_DEAD_MENU: {
        draw_cave_bg();

        LCD_printString("YOU DIED!", 48, 60, 2, 2);
        LCD_Draw_Rect(40, 90, 160, 2, 10, 1);

        char *opts[3] = {"PLAY AGAIN", "LEVEL SELECT", "MAIN MENU"};
        for (int i = 0; i < 3; i++) {
            int iy = 106 + i * 26;
            if (ge->cursor_pos == i) {
                LCD_Draw_Rect(44, iy - 2, 152, 16, 12, 1);
                LCD_printString(">", 50, iy, 6, 1);
                LCD_printString(opts[i], 64, iy, 1, 1);
            } else {
                LCD_printString(" ", 50, iy, 1, 1);
                LCD_printString(opts[i], 64, iy, 13, 1);
            }
        }

        LCD_printString("UP/DN=move  E=select", 20, 190, 13, 1);
        break;
    }

    case GS_WIN:
        draw_cave_bg();
        if (ge->level_idx + 1 >= NUM_LEVELS) {
            LCD_Draw_Rect(0, 0, 240, 240, 0, 1);
            LCD_Draw_Rect(10, 20, 220, 4, 10, 1);
            LCD_printString("YOU ESCAPED!", 20, 34, 10, 2);
            LCD_Draw_Rect(10, 58, 220, 4, 10, 1);
            LCD_Draw_Rect(10, 80, 220, 80, 9, 1);
            LCD_printString("Fireboy and Watergirl", 14, 88, 1, 1);
            LCD_printString("burst through the last", 14, 100, 1, 1);
            LCD_printString("portal as the ancient", 14, 112, 1, 1);
            LCD_printString("temple crumbled to", 14, 124, 1, 1);
            LCD_printString("dust behind them.", 14, 136, 1, 1);
            LCD_printString("LEGENDS.", 76, 160, 10, 1);
            LCD_Draw_Rect(10, 172, 220, 4, 10, 1);
            LCD_printString("All levels complete!", 14, 185, 3, 1);
            LCD_printString("Thanks for playing!", 18, 200, 13, 1);
        } else {
            LCD_printString("LEVEL CLEAR!", 24, 90, 3, 2);
            snprintf(buf, sizeof(buf), "Next: %s", g_lvl_names[ge->level_idx + 1]);
            LCD_printString(buf, 40, 130, 10, 1);
        }
        break;

    case GS_ABOUT: {
        draw_cave_bg();

        LCD_Draw_Rect(0, 0, 240, 20, 0, 1);
        LCD_Draw_Rect(0, 19, 240, 1, 10, 1);
        LCD_printString("ABOUT", 90, 5, 10, 1);
        LCD_printString("W=back", 180, 5, 13, 1);

        static const char *about_text[] = {
            "HOW TO PLAY",
            "",
            "FIREBOY  (left joystick)",
            "Left/Right  move",
            "Up          jump",
            "",
            "WATERGIRL (right joystick)",
            "Left/Right  move",
            "Up          jump",
            "SW button   also jumps",
            "SW (hold)   pause",
            "",
            "Both players must reach",
            "their EXIT door to win.",
            "Fireboy dies in water.",
            "Watergirl dies in lava.",
            "Geysers kill everyone.",
            "",
            "THE LEVELS",
            "",
            "1. Crystal Caves",
            "Hidden deep underground,",
            "crystal caves glow with",
            "ancient fire and water.",
            "Learn the basics here.",
            "",
            "2. Forest Temple",
            "An overgrown temple lost",
            "to the jungle. Platforms",
            "move and buttons open",
            "hidden bridges.",
            "",
            "3. Goblin Lair",
            "Watergirl is captured.",
            "Only Fireboy can move.",
            "Find the lever, dodge",
            "the goblin, and reach",
            "Watergirl to rescue her.",
            "",
            "4. Volcano's Heart",
            "Deep beneath the earth",
            "an ancient volcano stirs.",
            "Lava geysers erupt from",
            "the ground. Time every",
            "crossing or perish.",
            "",
            "5. The Final Run",
            "The temple is collapsing.",
            "A dark seal is broken.",
            "60 seconds. Ride moving",
            "platforms, pull levers,",
            "and escape together.",
            "",
            "Good luck.",
        };
        static const int about_x[] = {
             4, 4, 4,10,10, 4, 4,10,10,10,10, 4,
             4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
             4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
             4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
             4, 4, 4, 4, 4, 4, 4,
        };
        static const int about_col[] = {
            10, 1, 2, 1, 1, 1, 4, 1, 1, 1, 1, 1,
             1, 1, 2, 4, 6, 1,10, 1, 2, 1, 1, 1,
             1, 1, 3, 1, 1, 1, 1, 1, 6, 1, 1, 1,
             1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 5,
             1, 1, 1, 1, 1, 1,10,
        };
        int num_lines = 55;

        int y = 24 - ge->about_scroll;
        for (int i = 0; i < num_lines; i++) {
            int ly = y + i * 12;
            if (ly < 20 || ly > 236) continue;
            if (about_text[i][0] == '\0') continue;
            LCD_printString(about_text[i], about_x[i], ly, about_col[i], 1);
        }

        // scrollbar on right edge
        int total_h = num_lines * 12;
        int bar_h = (200 * 216) / (total_h > 1 ? total_h : 1);
        if (bar_h < 8) bar_h = 8;
        if (bar_h > 216) bar_h = 216;
        int bar_y = 20 + (ge->about_scroll * (216 - bar_h)) / (total_h > 216 ? total_h - 216 : 1);
        LCD_Draw_Rect(237, 20, 3, 220, 0, 1);
        LCD_Draw_Rect(237, bar_y, 3, bar_h, 10, 1);
        break;
    }

    default: break;
    }
}
