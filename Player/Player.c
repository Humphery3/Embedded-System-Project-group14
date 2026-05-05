#include "Player.h"

// lane centres at 40, 120, 200 - left edge = centre - 12 (half of 24px sprite width)
const int PLAYER_LANE_X[3] = {28, 108, 188};

// palette: 0=black  1=white(thobe)  2=red(ghutra)  10=skin  12=brown(sandals)
const uint8_t player_sprite[PLAYER_ROWS][PLAYER_COLS] = {
    {255, 255,   0,   0,   0,   0,   0, 255, 255, 255, 255, 255},
    {255,   0,   1,   2,   1,   2,   1,   0, 255, 255, 255, 255},
    {255,   0,   2,   1,   2,   1,   2,   0, 255, 255, 255, 255},
    {255,   0,  10,  10,  10,  10,   0, 255, 255, 255, 255, 255},
    {255, 255,   0,  10,  10,   0, 255, 255, 255, 255, 255, 255},
    {255,   0,   1,   1,   1,   1,   1,   0,   0, 255, 255, 255},
    {  0,   1,   1,   1,   1,   1,   1,   1,   1,   0, 255, 255},
    {255,   0,   1,   1,   1,   1,   1,   1,   0, 255, 255, 255},
    {255,   0,   1,   1,   1,   1,   1,   0, 255, 255, 255, 255},
    {255, 255,   0,   1,   1,   1,   1,   0, 255, 255, 255, 255},
    {255, 255,   0,   1,   1,   1,   0, 255, 255, 255, 255, 255},
    {255, 255, 255,   0,   1,   1,   0, 255, 255, 255, 255, 255},
    {255, 255, 255,   0,   1,   0, 255, 255, 255, 255, 255, 255},
    {255, 255,   0, 255, 255,   0, 255, 255, 255, 255, 255, 255},
    {255,   0,  12, 255, 255,  12,   0, 255, 255, 255, 255, 255},
    {255,   0,   0, 255, 255,   0,   0, 255, 255, 255, 255, 255},
};

static int s_prev_x = 0;
static int s_prev_y = 0;

void Player_Init(Player_t *p) {
    p->lane    = 1;
    p->jumping = false;
    p->sliding = false;
    p->jvel    = 0;
    p->jy      = 0;
    s_prev_x   = 0;
    s_prev_y   = 0;
}

void Player_Update(Player_t *p, float joyX, float joyY) {
    int xDir = 0;
    if      (joyX >  0.5f) xDir =  1;
    else if (joyX < -0.5f) xDir = -1;

    if (xDir != 0 && s_prev_x == 0) {  // edge-triggered so holding doesn't keep switching
        p->lane += xDir;
        if (p->lane < 0) p->lane = 0;
        if (p->lane > 2) p->lane = 2;
    }
    s_prev_x = xDir;

    int yDir = 0;
    if      (joyY >  0.5f) yDir =  1;
    else if (joyY < -0.5f) yDir = -1;

    if (yDir == 1 && s_prev_y != 1 && !p->jumping) {
        p->jumping = true;
        p->jvel    = JUMP_INIT_V;
        // p->jumpTimer = JUMP_FRAMES;  // old timer parabola
        p->jy      = 0;
        p->sliding = false;
    }
    s_prev_y = yDir;

    p->sliding = (!p->jumping && yDir == -1);

    // velocity instead of old parabola - double grav on descent stops it floating
    if (p->jumping) {
        p->jy   += p->jvel;
        p->jvel -= (p->jvel > 0) ? GRAV_UP : GRAV_DOWN;
        if (p->jy <= 0) {
            p->jumping = false;
            p->jy      = 0;
            p->jvel    = 0;
        }
    }
}
