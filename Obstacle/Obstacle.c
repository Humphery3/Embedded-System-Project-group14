#include "Obstacle.h"

void Obstacle_Init(Obstacle_t *o, ObstacleType_t type, int lane, int y) {
    o->type   = type;
    o->lane   = lane;
    o->y      = y;
    o->active = true;
    o->sprite = 0;
}

void Obstacle_Scroll(Obstacle_t *o, int speed) {
    o->y += speed;
}

bool Obstacle_OffScreen(Obstacle_t *o) {
    return o->y > OBS_COLLISION_EXIT + 5;
}

bool Obstacle_CollidesWithPlayer(const Obstacle_t *o,
                                 int playerLane,
                                 bool playerJumping,
                                 bool playerSliding) {
    if (!o->active)            return false;
    if (o->lane != playerLane) return false;

    // Only active during the y window where the obstacle overlaps the player
    if (o->y < OBS_COLLISION_ENTER || o->y > OBS_COLLISION_EXIT) return false;

    switch (o->type) {
        case OBS_BOULDER: return !playerJumping;   // jump over the boulder
        case OBS_BEAM:    return !playerSliding;   // slide under the beam barrier
        case OBS_WALL:    return true;             //  change lane to avoid the wall
    }
    return false;
}
