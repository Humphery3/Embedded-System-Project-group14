#ifndef LEVEL_H
#define LEVEL_H

#include <stdbool.h>

#define MAX_PLATFORMS   20
#define MAX_HAZARDS     10
#define MAX_BUTTONS      4
#define MAX_DIAMONDS     6
#define MAX_GOBLINS      4
#define GOBLIN_W        12
#define GOBLIN_H        16
#define GOBLIN_NORMAL    0
#define GOBLIN_BOULDER   1

typedef struct {int x, y, w, h; } Rect;

typedef enum { HAZARD_LAVA, HAZARD_WATER } HazardType;

typedef struct {
    int    x, y, w, h;
    HazardType type;
} Hazard;

typedef enum { PLAT_STATIC, PLAT_MOVING_H, PLAT_MOVING_V, PLAT_GEYSER } PlatformType;

typedef struct {
    int      x, y, w, h;
    PlatformType type;
    int      move_min, move_max;
    int       speed, dir;
    bool         active;
    bool         solid;  
} Platform;

typedef struct {
    int x, y, w, h;
    int target_platform;
    bool    pressed;
    bool    one_shot;   
} Button;

typedef struct {
    int x, y;
    bool    collected;
} Diamond;

typedef struct {
    int  x, y;
    int  vx;
    int  min_x, max_x;
    bool     active;
    bool     facing_right;
    int type;     
} Goblin;

typedef struct {
    Platform platforms[MAX_PLATFORMS]; int platform_count;
    Hazard   hazards[MAX_HAZARDS];     int hazard_count;
    Button   buttons[MAX_BUTTONS];     int button_count;
    Diamond  diamonds[MAX_DIAMONDS];   int diamond_count;
    Goblin   goblins[MAX_GOBLINS];     int goblin_count;
    bool     crumbles;
    Rect     exit_fire, exit_water;
    int  fire_spawn_x,  fire_spawn_y;
    int  water_spawn_x, water_spawn_y;
} Level;

void Level_Load(Level *lvl, int index);
void Level_Update(Level *lvl);
void Level_Draw(Level *lvl);
void Level_CheckButtons(Level *lvl,
                         int fb_x, int fb_y,
                         int wg_x, int wg_y);

#endif 
