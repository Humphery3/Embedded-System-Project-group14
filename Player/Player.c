#include "Player.h"
#include "Level.h"
#include "Sprites.h"
#include "game.h"

#define SCREEN_W 240

static bool overlap(int ax, int ay, int bx, int by, int bw, int bh) {
    AABB a = {ax, ay, PLAYER_W, PLAYER_HEIGHT};
    AABB b = {bx, by, bw, bh};
    return AABB_Collides(&a, &b);
}

static void advance_frame(Player *p, int max_frames) {
    p->anim_timer++;
    if (p->anim_timer >= ANIM_TICKS) {
        p->anim_timer = 0;
        p->frame++;
        if (p->frame >= max_frames) p->frame = 0;
    }
}

static bool near_wall(const Player *p, const Level *lvl) {
    int probe_x;
    if (p->facing_right) probe_x = p->x + PLAYER_W;
    else probe_x = p->x - PUSH_DIST;
    for (int i = 0; i < lvl->platform_count; i++) {
        Platform *pl = &lvl->platforms[i];
        if (!pl->active) continue;
        if (overlap(probe_x, p->y, pl->x, pl->y, pl->w, pl->h))
            return true;
    }
    return false;
}

void Player_Init(Player *p, PlayerType type, int x, int y) {
    p->x             = x;
    p->y             = y;
    p->vx            = 0;
    p->vy            = 0;
    p->on_ground     = false;
    p->facing_right  = true;
    p->type          = type;
    p->anim          = ANIM_IDLE;
    p->frame         = 0;
    p->grace_timer  = 0;
    p->anim_timer    = 0;
    p->ride_platform = -1;
}

void Player_Update(Player *p, UserInput input, Level *lvl) {
    bool left  = (input.direction == W  || input.direction == NW || input.direction == SW);
    bool right = (input.direction == E  || input.direction == NE || input.direction == SE);
    bool jump  = (input.direction == N  || input.direction == NW || input.direction == NE);

    // move with platform we stood on last frame
    if (p->ride_platform >= 0 && p->ride_platform < lvl->platform_count) {
        Platform *rp = &lvl->platforms[p->ride_platform];
        if (rp->active && rp->type == PLAT_MOVING_V)
            p->y += rp->speed * rp->dir;
        else if (rp->active && rp->type == PLAT_MOVING_H)
            p->x += rp->speed * rp->dir;
    }
    p->ride_platform = -1;

    // grace timer — lets you jump just after walking off a ledge
    if (p->on_ground) {
        p->grace_timer = GRACE_TICKS;
    } else if (p->grace_timer > 0) {
        p->grace_timer--;
    }

    if (left) {
        p->vx = -MOVE_SPEED;
        p->facing_right = false;
    } else if (right) {
        p->vx = MOVE_SPEED;
        p->facing_right = true;
    } else {
        p->vx = 0;
    }

    // jump only if grounded or within coyote window
    if (jump && (p->on_ground || p->grace_timer > 0)) {
        p->vy           = JUMP_VEL;
        p->on_ground    = false;
        p->grace_timer = 0;
    }

    p->vy += GRAVITY;

    // move x and resolve collisions
    p->x += p->vx;
    for (int i = 0; i < lvl->platform_count; i++) {
        Platform *pl = &lvl->platforms[i];
        if (!pl->active) continue;
        if (overlap(p->x, p->y, pl->x, pl->y, pl->w, pl->h)) {
            p->x -= p->vx;
            p->vx = 0;
        }
    }

    // move y and resolve collisions
    int prev_head = p->y;
    int prev_feet = p->y + PLAYER_HEIGHT;
    p->y += p->vy;
    p->on_ground = false;
    int curr_feet = p->y + PLAYER_HEIGHT;

    // landing
    for (int i = 0; i < lvl->platform_count; i++) {
        Platform *pl = &lvl->platforms[i];
        if (!pl->active || p->vy <= 0) continue;
        if (pl->type == PLAT_GEYSER) continue;
        if (p->x + PLAYER_W <= pl->x || p->x >= pl->x + pl->w) continue;
        if (prev_feet <= pl->y && curr_feet >= pl->y) {
            p->y             = pl->y - PLAYER_HEIGHT;
            p->on_ground     = true;
            p->vy            = 0;
            curr_feet        = p->y + PLAYER_HEIGHT;
            p->ride_platform = i;
        }
    }

    // ceiling
    if (p->vy < 0) {
        for (int i = 0; i < lvl->platform_count; i++) {
            Platform *pl = &lvl->platforms[i];
            if (!pl->active) continue;
            if (pl->type == PLAT_MOVING_V) continue;
            if (pl->type == PLAT_GEYSER) continue;
            if (p->x + PLAYER_W <= pl->x || p->x >= pl->x + pl->w) continue;
            int plat_bot = pl->y + pl->h;
            if (prev_head >= plat_bot && p->y < plat_bot) {
                p->y  = plat_bot;
                p->vy = 0;
            }
        }
    }

    // clamps to screen
    if (p->x < 0) p->x = 0;
    if (p->x > SCREEN_W - PLAYER_W) p->x = SCREEN_W - PLAYER_W;

    // animation
    AnimState new_anim;
    if (!p->on_ground) {
        new_anim = ANIM_JUMP;
    } else if (p->vx != 0) {
        if (near_wall(p, lvl)) new_anim = ANIM_PUSH;
        else new_anim = ANIM_RUN;
    } else {
        new_anim = ANIM_IDLE;
    }

    if (new_anim != p->anim) {
        p->anim       = new_anim;
        p->frame      = 0;
        p->anim_timer = 0;
    }

    bool is_fb = (p->type == PLAYER_FIRE);
    int max_frames;
    switch (p->anim) {
        case ANIM_IDLE: max_frames = is_fb ? FB_IDLE_FRAMES : WG_IDLE_FRAMES; break;
        case ANIM_RUN:  max_frames = is_fb ? FB_RUN_FRAMES  : WG_RUN_FRAMES;  break;
        case ANIM_JUMP: max_frames = is_fb ? FB_JUMP_FRAMES : WG_JUMP_FRAMES; break;
        case ANIM_PUSH: max_frames = is_fb ? FB_PUSH_FRAMES : WG_PUSH_FRAMES; break;
        default:        max_frames = 1; break;
    }
    if (max_frames > 0) advance_frame(p, max_frames);
}

void Player_Draw(const Player *p) {
    bool is_fb = (p->type == PLAYER_FIRE);
    bool flip  = !p->facing_right;
    const tImage *img = NULL;
    int f = p->frame;

    switch (p->anim) {
        case ANIM_IDLE: img = is_fb ? fb_idle_frames[0] : wg_idle_frames[0]; break;
        case ANIM_RUN:  img = is_fb ? fb_run_frames[f]  : wg_run_frames[f];  break;
        case ANIM_JUMP: img = is_fb ? fb_jump_frames[f] : wg_jump_frames[f]; break;
        case ANIM_PUSH: img = is_fb ? fb_push_frames[f] : wg_push_frames[f]; break;
    }

    if (img)
        Sprite_Blit(img, p->x + SPRITE_OFFSET_X, p->y + SPRITE_OFFSET_Y + TOP_BAR_H, flip);
}
