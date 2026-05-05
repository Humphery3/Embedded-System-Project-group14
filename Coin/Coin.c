#include "Coin.h"

void Coin_Init(Coin_t *c, int lane, int y) {
    c->lane      = lane;
    c->y         = y;
    c->collected = false;
}

void Coin_Scroll(Coin_t *c, int speed) {
    c->y += speed;
}

bool Coin_OffScreen(Coin_t *c) {
    return c->y > COIN_COLLECT_EXIT + 5;
}

bool Coin_CollidesWithPlayer(const Coin_t *c, int playerLane) {
    if (c->collected)          return false;
    if (c->lane != playerLane) return false;
    return (c->y >= COIN_COLLECT_ENTER && c->y <= COIN_COLLECT_EXIT);
}
