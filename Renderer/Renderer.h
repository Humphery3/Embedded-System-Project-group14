#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
#include "Player.h"
#include "Obstacle.h"
#include "Coin.h"
#include "ST7789V2_Driver.h"

void Renderer_Init(ST7789V2_cfg_t *cfg);

// per-frame draw calls
void Renderer_DrawBackground(bool day);
void Renderer_DrawPlayer(Player_t *p);
void Renderer_DrawObstacles(Obstacle_t *obstacles, int count);
void Renderer_DrawCoins(Coin_t *coins, int count);
void Renderer_DrawHUD(int score, int coins);

// state screens
void Renderer_DrawMenu(void);
void Renderer_DrawGameOver(int score, int coins);

void Renderer_Present(void);

#endif
