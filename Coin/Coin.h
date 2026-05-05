#ifndef COIN_H
#define COIN_H

#include <stdint.h>
#include <stdbool.h>
#include "Player.h"

#define COIN_RADIUS  4

#define COIN_COLLECT_ENTER  (PLAYER_Y - 22)
#define COIN_COLLECT_EXIT   (PLAYER_Y + 8)

typedef struct {
    int  lane;
    int  y;
    bool collected;
} Coin_t;

void Coin_Init(Coin_t *c, int lane, int y);
void Coin_Scroll(Coin_t *c, int speed);
bool Coin_OffScreen(Coin_t *c);
bool Coin_CollidesWithPlayer(const Coin_t *c, int playerLane);

#endif
