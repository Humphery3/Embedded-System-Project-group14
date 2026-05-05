#ifndef PLAYER_H
#define PLAYER_H

#include "Joystick.h"
#include "Utils.h"
#include "Sprites.h"
#include "game.h"
#include "Level.h"
#include <stdbool.h>

#define PLAYER_W        20
#define PLAYER_HEIGHT   30
#define SPRITE_OFFSET_X (-8)
#define SPRITE_OFFSET_Y (-6)
#define GRAVITY         1
#define JUMP_VEL        (-10)
#define JUMP_VEL_MIN    (-5)
#define MOVE_SPEED      4
#define ANIM_TICKS      6
#define PUSH_DIST       4
#define GRACE_TICKS    8

typedef enum { PLAYER_FIRE, PLAYER_WATER } PlayerType;
typedef enum { ANIM_IDLE, ANIM_RUN, ANIM_JUMP, ANIM_PUSH } AnimState;

typedef struct {
    int        x, y;
    int        vx, vy;
    bool       on_ground;
    bool       facing_right;
    PlayerType type;

    AnimState  anim;
    int        frame;
    int        anim_timer;
    int        grace_timer;
    int        ride_platform;
} Player;

void Player_Init(Player *p, PlayerType type, int x, int y);
void Player_Update(Player *p, UserInput input, Level *lvl);
void Player_Draw(const Player *p);

#endif
