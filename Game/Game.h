#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include "Obstacle.h"
#include "Coin.h"

#define MAX_OBSTACLES 6
#define MAX_COINS     4

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAMEOVER
} GameState_t;

typedef struct {
    int         score;
    int         coins;
    float       speed;
    int         tick;
    bool        day;
    GameState_t state;
    int         lvl;   // 0-9, drives spawn rate and wave frequency

    Obstacle_t  obstacles[MAX_OBSTACLES];
    Coin_t      coinPool[MAX_COINS];
    int         obs_spawn_timer;
    int         coin_spawn_timer;
} Game_t;

void Game_Init(Game_t *g);
void Game_Update(Game_t *g, int playerLane, bool playerJumping, bool playerSliding);

#endif
