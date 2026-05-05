#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <stdint.h>
#include <stdbool.h>

#define OBS_W_MAX    28
#define OBS_H_ROCK   22
#define OBS_H_BEAM   14
#define OBS_H_WALL   42

// collision window - must stay in sync with PLAYER_Y = 200 in Player.h
#define OBS_PLAYER_FOOT_Y   200
#define OBS_COLLISION_ENTER (OBS_PLAYER_FOOT_Y - 18)   // = 182
#define OBS_COLLISION_EXIT  (OBS_PLAYER_FOOT_Y + 8)    // = 208

typedef enum {
    OBS_BOULDER,   // ground rock  - jump to clear
    OBS_BEAM,      // low beam     - slide to clear
    OBS_WALL       // full wall    - change lane (can't jump or slide past)
} ObstacleType_t;

typedef struct {
    ObstacleType_t  type;
    int             lane;
    int             y;       // grows toward player each frame
    bool            active;
    const uint8_t  *sprite;  // TODO: add sprite data per type
} Obstacle_t;

void Obstacle_Init(Obstacle_t *o, ObstacleType_t type, int lane, int y);
void Obstacle_Scroll(Obstacle_t *o, int speed);
bool Obstacle_OffScreen(Obstacle_t *o);
bool Obstacle_CollidesWithPlayer(const Obstacle_t *o, int playerLane,
                                 bool playerJumping, bool playerSliding);

#endif
