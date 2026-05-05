#include "Level.h"
#include "LCD.h"
#include <string.h>

#define TOP_BAR 20   // 20 pixels reserved for Game UI

static void fill(int x, int y, int w, int h, int c) {
    LCD_Draw_Rect(x, y + TOP_BAR, w, h, c, 1);
}
static void outline(int x, int y, int w, int h, int c) {
    LCD_Draw_Rect(x, y + TOP_BAR, w, h, c, 0);
}
static void hline(int x, int y, int w, int c) {
    LCD_Draw_Line(x, y + TOP_BAR, x + w - 1, y + TOP_BAR, c);
}
static void vline(int x, int y, int h, int c) {
    LCD_Draw_Line(x, y + TOP_BAR, x, y + h - 1 + TOP_BAR, c);
}
static void px(int x, int y, int c) {
    LCD_Set_Pixel(x, y + TOP_BAR, c);
}

typedef struct { int x, y; } DiamondPos;
typedef struct { int x, y, vx, min_x, max_x, type; } GoblinInit;

typedef struct {
    const Platform *plats; int np;
    const Hazard *haz; int nh;
    const Button *btns; int nb;
    int fsx, fsy, wsx, wsy;
    Rect ef, ew;
    const DiamondPos *gems; int ng;
    const GoblinInit *gobs; int ngg;
    bool crumbles;
} LvlDesc;

// Level 1 — Crystal Caves 
static const Platform l0p[] = {
    {  0, 210, 240, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {176, 210,  64, 8, PLAT_STATIC, 0, 0, 0, 1, false,  true},
    {116, 210,  24, 8, PLAT_STATIC, 0, 0, 0, 1, false,  true},
    {  0, 168,  60, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {  0, 126, 180, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {140, 126, 100, 8, PLAT_STATIC, 0, 0, 0, 1, false,  true},
    {  0,  84,  80, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {195, 210,  30, 8, PLAT_MOVING_V, 126, 210, 2, 1, true, true},
    {165,  84,  75, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {  0,  42, 110, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {155,  42,  85, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    { 80,  84,  85, 8, PLAT_STATIC,   0,  0, 0, 1, false,  true},
    { 80,  84,  25, 8, PLAT_MOVING_H, 80, 140, 2, 1, true, true},
};
static const Hazard l0h[] = {
    { 80, 210,  36, 8, HAZARD_LAVA },
    {140, 210,  36, 8, HAZARD_WATER},
};

static const Button l0b[] = {
    {178, 194, 14, 8,  7, false, false},  // B1: button to raise platform     
    {165, 110, 14, 8,  7, false, false},  // B2: button to raise platform
    {  5,  68, 14, 8, 11, false,  true},  // B3: lever to activate platform
};
static const DiamondPos l0d[] = {
    {120, 155},
    {180,  30},
};

// Level 2 — Forest Temple

static const Platform l1p[] = {
    {  0, 210, 240,  8, PLAT_STATIC,    0,   0, 0,  1,  true, true},
    {  0, 168,  85,  8, PLAT_STATIC, 168, 168, 0,  1,  true, true},
    {110, 168, 105,  8, PLAT_STATIC, 168, 168, 0,  1,  true, true},
    {  0, 126, 130,  8, PLAT_MOVING_V, 126, 126, 0,  1,  true, true},
    {145, 126,  70,  8, PLAT_MOVING_V, 126, 126, 0,  1,  true, true},
    {  0,  84, 100,  8, PLAT_STATIC,  84,  84, 0,  1,  true, true},
    {125,  84,  90,  8, PLAT_STATIC,  84,  84, 0,  1,  true, true},
    {  0,  42,  80,  8, PLAT_MOVING_V,  42,  42, 0,  1,  true, true},
    {160,  42,  80,  8, PLAT_MOVING_V,  42,  42, 0,  1,  true, true},
    { 80,  42,  80,  8, PLAT_MOVING_V,  42,  42, 0,  1, false, true},
    {215, 168,  20,  8, PLAT_STATIC,  84, 168, 2, -1, false, true},
};
static const Hazard l1h[] = {
    {  5, 168, 50, 8, HAZARD_LAVA },
    {135, 168, 55, 8, HAZARD_WATER},
    {  5, 126, 50, 8, HAZARD_LAVA },
    {150, 126, 65, 8, HAZARD_WATER},
    {  5,  84, 50, 8, HAZARD_LAVA },
    {  5,  42, 50, 8, HAZARD_LAVA },
};
static const Button l1b[] = {
    { 65, 152, 14, 8, 10, false, false},
    { 30,  26, 14, 8,  9, false,  true},
};
static const DiamondPos l1d[] = {{52, 69}, {175, 111}};
static const GoblinInit l1g[] = {
    {  5, 152,  2,   5,  70, GOBLIN_NORMAL},
    {130,  68,  2, 128, 210, GOBLIN_NORMAL},
};

// Level 3 — Goblin Lair
static const Platform l2p[] = {
    {  0, 210, 240, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {  0, 166,  90, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {165, 166,  75, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {  0, 124,  90, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {135, 124, 105, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {  0,  82,  65, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    { 65,  82, 105, 8, PLAT_STATIC, 0, 0, 0, 1, false,  true},
    {170,  82,  70, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {  0,  40, 120, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
    {165,  40,  75, 8, PLAT_STATIC, 0, 0, 0, 1,  true,  true},
};
static const Hazard l2h[] = {
    {  0, 124, 56, 8, HAZARD_WATER},
    {150, 210, 50, 8, HAZARD_WATER},
};
// lever on tier3 left activates bridge p6
static const Button l2b[] = {
    {5, 68, 14, 8, 6, false, true},
};
static const GoblinInit l2g[] = {
    {165, 150,  2, 165, 225, GOBLIN_NORMAL},
    {  5,  24,  2,   5, 110, GOBLIN_NORMAL},
};

static const DiamondPos l2d[] = {{0, 0}};  

// Level 4 — Volcano's Heart
static const Platform l3p[] = {
    {  0, 210, 240, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    {  0, 168, 120, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    {150, 168,  90, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    {  0, 126, 110, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    {140, 126, 100, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    {  0,  84, 110, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    {  0,  42,  55, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    {155,  42,  85, 8, PLAT_STATIC,    0,   0, 0,  1,  true,  true},
    { 55,  42, 100, 8, PLAT_STATIC,    0,   0, 0,  1, false,  true},
    { 70, 168,  10, 8, PLAT_GEYSER,  132, 168, 2, -1,  true,  true},
    {180, 126,  10, 8, PLAT_GEYSER,   90, 126, 2, -1,  true,  true},
    {155, 126,  25, 8, PLAT_MOVING_H,155, 210, 2,  1,  true,  true},
};
static const Hazard l3h[] = {
    { 85, 168, 28, 8, HAZARD_LAVA },
    {140, 126, 28, 8, HAZARD_WATER},
};
// lever on summit-L activates top bridge p8
static const Button l3b[] = {
    {5, 68, 14, 8, 8, false, true},
};
static const DiamondPos l3d[] = {{0, 0}};
static const GoblinInit l3g[] = {
    { 30,  68,  2,  30, 100, GOBLIN_NORMAL},
    {160, 110,  2, 155, 225, GOBLIN_NORMAL},
};

// Level 5 — The Final Run
 
static const Platform l4p[] = {
    {  0, 210, 240, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    {  0, 168,  90, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    { 90, 168,  50, 8, PLAT_MOVING_H, 90, 125, 2, 1,  true,  true},
    {160, 168,  80, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    {  0, 126,  80, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    {135, 126, 105, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    {  0,  84,  65, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    { 65,  84, 110, 8, PLAT_STATIC,    0,   0, 0, 1, false,  true},
    {175,  84,  65, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    {  0,  42, 110, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
    {155,  42,  85, 8, PLAT_STATIC,    0,   0, 0, 1,  true,  true},
};
static const Hazard l4h[] = {
    { 55, 168,  35, 8, HAZARD_LAVA },
    {160, 168,  55, 8, HAZARD_WATER},
};
// lever on tier3 left activates bridge p7
static const Button l4b[] = {
    {5, 68, 14, 8, 7, false, true},
};
static const GoblinInit l4g[] = {
    {  5, 110,  2,   5,  70, GOBLIN_NORMAL },
    {140, 110, -2, 140, 215, GOBLIN_BOULDER},
    {175,  68,  2, 175, 230, GOBLIN_NORMAL },
    {160,  26,  2, 160, 225, GOBLIN_NORMAL },
};
static const DiamondPos l4d[] = {{0, 0}};

static const LvlDesc table[] = {
    // LVL 1
    {l0p, 13, l0h, 2, l0b, 3,   5, 180,  26, 180,
     {5, 26, 14, 16}, {221, 26, 14, 16}, l0d, 2, NULL, 0, false},
    // LVL 2
    {l1p, 11, l1h, 6, l1b, 2,  10, 180, 160, 180,
     {5, 26, 14, 16}, {162, 26, 14, 16}, l1d, 2, l1g, 2, false},
    // LVL 3
    {l2p, 10, l2h, 2, l2b, 1,   5, 180, 210,  10,
     {170, 8, 24, 22}, {210, 8, 14, 22}, l2d, 0, l2g, 2, false},
    // LVL 4
    {l3p, 12, l3h, 2, l3b, 1,   5, 138, 200, 138,
     {5, 26, 14, 16}, {221, 26, 14, 16}, l3d, 0, l3g, 2, false},
    // LVL 5
    {l4p, 11, l4h, 2, l4b, 1,   5, 180, 215, 180,
     {5, 8, 24, 22}, {210, 8, 24, 22}, l4d, 0, l4g, 4, true},
};

void Level_Load(Level *lvl, int idx) {
    const LvlDesc *d = &table[idx];
    memset(lvl, 0, sizeof(Level));
    lvl->platform_count = d->np;
    for (int i = 0; i < d->np; i++) lvl->platforms[i] = d->plats[i];
    lvl->hazard_count = d->nh;
    for (int i = 0; i < d->nh; i++) lvl->hazards[i] = d->haz[i];
    lvl->button_count = d->nb;
    for (int i = 0; i < d->nb; i++) lvl->buttons[i] = d->btns[i];
    lvl->diamond_count = d->ng;
    for (int i = 0; i < d->ng; i++) {
        lvl->diamonds[i].x = d->gems[i].x;
        lvl->diamonds[i].y = d->gems[i].y;
        lvl->diamonds[i].collected = false;
    }
    lvl->goblin_count = d->ngg;
    for (int i = 0; i < d->ngg; i++) {
        lvl->goblins[i].x = d->gobs[i].x;
        lvl->goblins[i].y = d->gobs[i].y;
        lvl->goblins[i].vx = d->gobs[i].vx;
        lvl->goblins[i].min_x = d->gobs[i].min_x;
        lvl->goblins[i].max_x = d->gobs[i].max_x;
        lvl->goblins[i].active = true;
        lvl->goblins[i].facing_right = (d->gobs[i].vx > 0);
        lvl->goblins[i].type = d->gobs[i].type;
    }
    lvl->crumbles = d->crumbles;
    lvl->fire_spawn_x = d->fsx;
    lvl->fire_spawn_y = d->fsy;
    lvl->water_spawn_x = d->wsx;
    lvl->water_spawn_y = d->wsy;
    lvl->exit_fire = d->ef;
    lvl->exit_water = d->ew;
}

void Level_Update(Level *lvl) {
    for (int i = 0; i < lvl->platform_count; i++) {
        Platform *p = &lvl->platforms[i];
        if (!p->active || p->type == PLAT_STATIC) continue;
        if (p->type == PLAT_MOVING_H) {
            p->x += p->speed * p->dir;
            if (p->x >= p->move_max) { p->x = p->move_max; p->dir = -1; }
            if (p->x <= p->move_min) { p->x = p->move_min; p->dir =  1; }
        } else if (p->type == PLAT_GEYSER) {
            p->y += p->speed * p->dir;
            if (p->y <= p->move_min) { p->y = p->move_min; p->dir =  1; }
            if (p->y >= p->move_max) { p->y = p->move_max; p->dir = -1; }
        } else {
            p->y += p->speed * p->dir;
            if (p->y < p->move_min) p->y = p->move_min;
            if (p->y > p->move_max) p->y = p->move_max;
        }
    }
    for (int i = 0; i < lvl->goblin_count; i++) {
        Goblin *g = &lvl->goblins[i];
        if (!g->active) continue;
        g->x += g->vx;
        if (g->vx > 0 && g->x >= g->max_x) { g->x = g->max_x; g->vx = -g->vx; }
        if (g->vx < 0 && g->x <= g->min_x) { g->x = g->min_x; g->vx = -g->vx; }
        g->facing_right = (g->vx > 0);
    }
}

void Level_CheckButtons(Level *lvl,
                         int fx, int fy,
                         int wx, int wy) {
    for (int i = 0; i < lvl->button_count; i++) {
        Button *b = &lvl->buttons[i];
        if (!b->one_shot && b->target_platform < lvl->platform_count) {
            Platform *pl = &lvl->platforms[b->target_platform];
            if (pl->type == PLAT_MOVING_V) pl->dir = 1;
        }
    }

    for (int i = 0; i < lvl->button_count; i++) {
        Button *b = &lvl->buttons[i];
        if (b->one_shot && b->pressed) continue;
        bool fo = fx < b->x+b->w && fx+12 > b->x && fy+16 >= b->y && fy+16 <= b->y+b->h+2;
        bool wo = wx < b->x+b->w && wx+12 > b->x && wy+16 >= b->y && wy+16 <= b->y+b->h+2;
        bool now = fo || wo;
        b->pressed = now;
        if (b->target_platform >= lvl->platform_count) continue;
        Platform *pl = &lvl->platforms[b->target_platform];
        if (b->one_shot) {
            if (now) pl->active = true;
        } else if (pl->type == PLAT_MOVING_V) {
            if (now) pl->dir = -1;
        } else {
            pl->active = now;
        }
    }
}

// stone tile
static const uint8_t tile_stone[64] = {
     1,  1,  1,  1,  1,  1,  1,  1,
    13, 13, 13, 13, 13, 13, 13, 13,
    12, 12, 12, 12, 12, 12, 12, 12,
    12, 12,  0, 12, 12, 12,  0, 12,
    12, 12, 12,  0, 12, 12, 12,  0,
    12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12,
     0,  0,  0,  0,  0,  0,  0,  0
};

// chain/moving platform tile
static const uint8_t tile_chain[64] = {
     6,  6,  6,  6,  6,  6,  6,  6,
     5,  5,  6,  5,  5,  6,  5,  5,
     5,  5,  5,  5,  5,  5,  5,  5,
     5,  5,  5,  5,  5,  5,  5,  5,
     5,  5,  5,  5,  5,  5,  5,  5,
     5,  5,  5,  5,  5,  5,  5,  5,
    12,  5, 12,  5, 12,  5, 12,  5,
    12, 12, 12, 12, 12, 12, 12, 12
};

// lava tile
static const uint8_t tile_lava[64] = {
     6,  5,  6,  5,  6,  5,  6,  5,
     5,  5,  6,  5,  5,  6,  5,  5,
     2,  5,  2,  2,  5,  2,  2,  5,
     2,  2,  2,  2,  2,  2,  2,  2,
     2,  2,  5,  2,  2,  2,  5,  2,
     2,  2,  2,  2,  2,  2,  2,  2,
     2,  2,  2,  2,  2,  2,  2,  2,
    12, 12, 12, 12, 12, 12, 12, 12
};

// water tile
static const uint8_t tile_water[64] = {
    14,  1, 14, 14,  1, 14, 14,  1,
     4,  4, 14,  4,  4, 14,  4,  4,
     4,  4,  4,  4,  4,  4,  4,  4,
     4,  9,  4,  4,  9,  4,  4,  9,
     9,  9,  4,  9,  9,  4,  9,  9,
     9,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  9,  9,  9,  9,  9,  9,
     0,  0,  0,  0,  0,  0,  0,  0
};

// horizontal tiles
static void tile_h(int x, int y, int w, int h,
                   const uint8_t *tile, int body_col) {
    int tx = x;
    for (; tx + 8 <= x + w; tx += 8)
        LCD_Draw_Sprite(tx, y + TOP_BAR, h, 8, tile);
    if (tx < x + w)
        fill(tx, y, x + w - tx, h, body_col);
}

// Draw Boulder
static void draw_boulder(int x, int y) {
    fill(x+1,  y+1,  10, 12, 12); 
    fill(x+3,  y,     6,  1, 13);  
    fill(x+2,  y+1,   2,  2, 13);
    fill(x+8,  y+2,   2,  3,  0);  
    fill(x+4,  y+5,   3,  2, 13); 
    fill(x,    y+6,   1,  4,  0);  
    fill(x+11, y+4,   1,  5,  0);  
    fill(x+2,  y+10,  8,  2,  0); 
}

//draw lever
static void draw_lever(int x, int y, int w, int h, bool pulled) {
    fill(x, y+4, w, h-4, 12);          
    outline(x, y+4, w, h-4, 0);
    int cx = x + w/2;
    if (pulled) {
        fill(cx,   y+2, 3, 2, 3);       
        fill(cx+1, y+1, 3, 2, 3);    
        fill(cx+2, y,   3, 3, 3);     
    } else {
        fill(cx-1, y,   3, 5, 6);     
        fill(cx-1, y,   3, 3, 6);     
    }
}

// draw goblin sprite
static void draw_goblin(int x, int y, bool facing_right) {
    fill(x+4,  y-4,  4, 2,  9);              
    fill(x+5,  y-7,  2, 4,  9);              
    fill(x+1,  y,   10, 7,  3);               
    int ex = facing_right ? x+6 : x+2;
    fill(ex,   y+2,  3, 2,  6);               
    fill(x+2,  y+5,  7, 1,  1);              
    fill(x+2,  y+7,  8, 5,  9);          
    fill(x,    y+7,  2, 4,  3);               
    fill(x+10, y+7,  2, 4,  3);              
    fill(x+2,  y+12, 3, 4, 12);               
    fill(x+7,  y+12, 3, 4, 12);            
}

// draw lava geyser column
static void draw_geyser(int x, int y, int w, int base_y) {
    int col_h = base_y - y;
    if (col_h <= 0) return;
    fill(x+1, y, w-2, col_h, 5);
    fill(x+2, y, w-4, col_h, 6);
    fill(x,   y,   w, 3, 6);     
    fill(x+1, y+3, w-2, 3, 5);    
    fill(x-2, y+2,  2, col_h/3, 5);
    fill(x+w, y+2,  2, col_h/3, 5);
    fill(x-1, base_y-4, w+2, 4, 12);
    outline(x-1, base_y-4, w+2, 4, 0);
}

// draw static stone platform
static void draw_plat_static(int x, int y, int w, int h) {
    tile_h(x, y, w, h, tile_stone, 12);
    vline(x,     y, h, 13);          
    vline(x+w-1, y, h, 0);      
}

// draw moving platform
static void draw_plat_moving(int x, int y, int w, int h) {
    tile_h(x, y, w, h, tile_chain, 5);
    vline(x,     y, h, 6);           
    vline(x+w-1, y, h, 12);           
}

// draw lava hazard
static void draw_lava(int x, int y, int w, int h) {
    hline(x+6, y-3, w-12, 5);
    hline(x+3, y-2, w-6,  5);
    hline(x,   y-1, w,    5);
    //body
    tile_h(x, y, w, h, tile_lava, 2);
    // border
    vline(x,     y-3, h+3, 12);
    vline(x+w-1, y-3, h+3, 12);
    hline(x,     y+h-1, w,  12);
}

// draw water hazard
static void draw_water(int x, int y, int w, int h) {
    hline(x+6, y-3, w-12, 9);
    hline(x+3, y-2, w-6,  4);
    hline(x,   y-1, w,    4);
    // body
    tile_h(x, y, w, h, tile_water, 9);
    // border
    vline(x,     y-3, h+3, 0);
    vline(x+w-1, y-3, h+3, 0);
    hline(x,     y+h-1, w,  0);
}

// draw fire exit portal
static void draw_exit_fire(int x, int y, int w, int h) {
    outline(x-1, y-1, w+2, h+2, 5);  
    fill(x, y, w, h, 0);              
    outline(x,   y,   w,   h,   12);  
    outline(x+1, y+1, w-2, h-2, 5);  
    outline(x+2, y+2, w-4, h-4, 6);   
    hline(x+2, y+2, w-4, 6);       
    hline(x+2, y+3, w-4, 5);         
    px(x+w/2,   y+h/2-1, 6);          
    px(x+w/2-1, y+h/2,   5);
    px(x+w/2,   y+h/2,   5);
    px(x+w/2+1, y+h/2,   5);
}

// draw water exit portal
static void draw_exit_water(int x, int y, int w, int h) {
    outline(x-1, y-1, w+2, h+2, 14);  
    fill(x, y, w, h, 0);             
    outline(x,   y,   w,   h,   0);  
    outline(x+1, y+1, w-2, h-2, 9);  
    outline(x+2, y+2, w-4, h-4, 4);  
    hline(x+2, y+2, w-4, 14);        
    hline(x+2, y+3, w-4, 4);          
    px(x+w/2,   y+h/2-1, 14);        
    px(x+w/2-1, y+h/2,   4);
    px(x+w/2,   y+h/2,   4);
    px(x+w/2+1, y+h/2,   4);
}

// draw button
static void draw_button(int x, int y, int w, int h, bool pressed) {
    int base = pressed ? 3 : 13;
    int sh   = pressed ? 9 : 0;
    outline(x-1, y-1, w+2, h+2, 0);  
    fill(x, y, w, h, base);
    hline(x,     y,     w, 1);
    vline(x,     y,     h, 1);
    hline(x,     y+h-1, w, sh);
    vline(x+w-1, y,     h, sh);
}

// draw diamond gem
static void draw_gem(int x, int y) {
    outline(x-1, y-1, 10, 9, 6);     
    px(x+3, y+0, 1);
    hline(x+2, y+1, 4, 10);
    hline(x+1, y+2, 6, 10);
    hline(x+0, y+3, 8, 10);
    hline(x+1, y+4, 6, 10);
    hline(x+2, y+5, 4, 10);
    px(x+3, y+6, 10);
    px(x+2, y+1, 6);                 
    px(x+5, y+3, 1);       
}

void Level_Draw(Level *lvl) {
    // draw cave background
    fill(0, 0, 240, 220, 9);
    for (int wy = 18; wy < 215; wy += 30) {
        hline(0, wy,   240, 12);
        hline(0, wy+1, 240, 0);
    }
    for (int brow = 0; brow < 8; brow++) {
        int xoff = (brow % 2 == 0) ? 0 : 30;
        int ytop = brow * 30;
        int ybot = ytop + 27;
        if (ybot > 219) ybot = 219;
        for (int bx = xoff; bx < 240; bx += 60) {
            vline(bx, ytop, ybot - ytop, 0);
        }
    }

    // draw level elements
    for (int i = 0; i < lvl->platform_count; i++) {
        Platform *p = &lvl->platforms[i];
        if (!p->active) continue;
        if      (p->type == PLAT_STATIC)  draw_plat_static(p->x, p->y, p->w, p->h);
        else if (p->type == PLAT_GEYSER)  draw_geyser(p->x, p->y, p->w, p->move_max);
        else                              draw_plat_moving(p->x, p->y, p->w, p->h);
    }
    for (int i = 0; i < lvl->hazard_count; i++) {
        Hazard *h = &lvl->hazards[i];
        if (h->type == HAZARD_LAVA) draw_lava (h->x, h->y, h->w, h->h);
        else                        draw_water(h->x, h->y, h->w, h->h);
    }
    for (int i = 0; i < lvl->button_count; i++) {
        Button *b = &lvl->buttons[i];
        if (b->one_shot) draw_lever(b->x, b->y, b->w, b->h, b->pressed);
        else             draw_button(b->x, b->y, b->w, b->h, b->pressed);
    }
    draw_exit_fire (lvl->exit_fire.x,  lvl->exit_fire.y,
                    lvl->exit_fire.w,  lvl->exit_fire.h);
    draw_exit_water(lvl->exit_water.x, lvl->exit_water.y,
                    lvl->exit_water.w, lvl->exit_water.h);
    for (int i = 0; i < lvl->diamond_count; i++) {
        if (!lvl->diamonds[i].collected)
            draw_gem(lvl->diamonds[i].x, lvl->diamonds[i].y);
    }
    for (int i = 0; i < lvl->goblin_count; i++) {
        Goblin *g = &lvl->goblins[i];
        if (!g->active) continue;
        if (g->type == GOBLIN_BOULDER) draw_boulder(g->x, g->y);
        else                           draw_goblin(g->x, g->y, g->facing_right);
    }
}
