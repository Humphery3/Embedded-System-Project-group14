#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#define PLAYER_ROWS   16
#define PLAYER_COLS   12
#define PLAYER_SCALE   2
#define PLAYER_Y     200

extern const int PLAYER_LANE_X[3];

// tuned by feel - 8 cleared boulders cleanly, 10 was too easy
#define JUMP_INIT_V  8
#define GRAV_UP      1
#define GRAV_DOWN    2   // falls faster than it rises, stops the floaty feeling

typedef struct {
    int  lane;
    bool jumping;
    bool sliding;
    int  jvel;   // upward px/frame, goes negative on the way down
    int  jy;     // pixels off the ground
} Player_t;

// 4-bit palette: 255 = transparent
extern const uint8_t player_sprite[PLAYER_ROWS][PLAYER_COLS];

void Player_Init(Player_t *p);
void Player_Update(Player_t *p, float joyX, float joyY);

#endif
