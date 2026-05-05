#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include <stdint.h>
#include <stdbool.h>
#include "Player.h"
#include "Level.h"

typedef enum {
    GS_TITLE,
    GS_LEVEL_SELECT,
    GS_SETTINGS,
    GS_ABOUT,
    GS_LOADING,
    GS_STORY,
    GS_PLAY,
    GS_PAUSE,
    GS_DEAD,
    GS_DEAD_MENU,
    GS_WIN,
} GameState;

#define NUM_LEVELS      5
#define DEAD_PAUSE_MS   1500
#define WIN_PAUSE_MS    2000
#define CRUMBLE_TICKS   1800
#define STORY_PAGES     3
#define STORY_PAGE_MS   4000

typedef struct {
    Player    fireboy;
    Player    watergirl;
    Level     level;
    GameState state;
    int       level_idx;
    int       crumble_timer;
    uint32_t  state_enter_ms;
    int       fb_diamonds;
    int       wg_diamonds;
    uint32_t  level_start_ms;

    // menu UI
    int       cursor_pos;
    int       sel_level;
    int       volume;
    int       speech_idx;
    int       loading_ticks;
    int       prev_dir;

    // story mode
    int       story_page;

    // about screen
    int       about_scroll;
} GameEngine;

void GameEngine_Init(GameEngine *ge);

void GameEngine_Update(GameEngine *ge, UserInput fb_in, UserInput wg_in, bool pause_btn);

void GameEngine_DrawScene(GameEngine *ge);

#endif 
