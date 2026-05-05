#include "Game.h"
#include "rng.h"

#define SPEED_RAMP  150   // every 5 s at 30 fps
#define SPEED_MAX  12.0f
#define LVL_RAMP   300   // every 10 s
#define LVL_MAX      9
#define DAY_FLIP   200

// TODO: bump buzzer pitch when lvl increases

#define SPAWN_Y  40

static uint32_t rng_next(void) {
    uint32_t v = 0;
    HAL_RNG_GenerateRandomNumber(&hrng, &v);
    return v;
}

// smaller divisor = waves more often - found 5 was too rare at the start
static const uint8_t wave_div[10] = { 5, 5, 4, 4, 4, 3, 3, 3, 2, 2 };

static int spawn_gap(Game_t *g, bool wave) {
    int base = wave ? 45 : 35;
    base -= (int)(g->speed * 2);
    base -= g->lvl * 2;
    int min = wave ? 10 : 6;
    if (base < min) base = min;
    int jitter = 18 - g->lvl * 2;
    if (jitter < 4) jitter = 4;
    return base + (int)(rng_next() % (uint32_t)jitter);
}

static bool lane_clear(Game_t *g, int lane) {
    for (int i = 0; i < MAX_OBSTACLES; i++)
        if (g->obstacles[i].active && g->obstacles[i].lane == lane)
            return false;
    return true;
}

void Game_Init(Game_t *g) {
    g->score  = 0;
    g->coins  = 0;
    g->speed  = 3.0f;
    g->tick   = 0;
    g->day    = true;
    g->lvl    = 0;
    g->state  = STATE_MENU;

    for (int i = 0; i < MAX_OBSTACLES; i++) g->obstacles[i].active   = false;
    for (int i = 0; i < MAX_COINS;     i++) g->coinPool[i].collected = true;

    g->obs_spawn_timer  = 15;
    g->coin_spawn_timer = 50;
}

void Game_Update(Game_t *g, int playerLane, bool playerJumping, bool playerSliding) {
    if (g->state != STATE_PLAYING) return;

    g->tick++;

    if ((g->tick % 5) == 0) g->score++;  // ~6 pts/sec at 30fps

    if ((g->tick % SPEED_RAMP) == 0 && g->speed < SPEED_MAX)
        g->speed += 1.0f;

    if ((g->tick % LVL_RAMP) == 0 && g->lvl < LVL_MAX)
        g->lvl++;

    if ((g->tick % DAY_FLIP) == 0)
        g->day = !g->day;

    int ispeed = (int)g->speed;

    for (int i = 0; i < MAX_OBSTACLES; i++) {
        Obstacle_t *o = &g->obstacles[i];
        if (!o->active) continue;

        Obstacle_Scroll(o, ispeed);

        if (Obstacle_OffScreen(o)) { o->active = false; continue; }

        if (Obstacle_CollidesWithPlayer(o, playerLane, playerJumping, playerSliding)) {
            g->state = STATE_GAMEOVER;
            return;
        }
    }

    for (int i = 0; i < MAX_COINS; i++) {
        Coin_t *c = &g->coinPool[i];
        if (c->collected) continue;

        Coin_Scroll(c, ispeed);

        if (Coin_OffScreen(c)) { c->collected = true; continue; }

        if (Coin_CollidesWithPlayer(c, playerLane)) {
            c->collected = true;
            g->coins++;
        }
    }

    g->obs_spawn_timer--;
    if (g->obs_spawn_timer <= 0) {
        int occ = 0;
        for (int l = 0; l < 3; l++)
            if (!lane_clear(g, l)) occ++;

        if (occ == 0 && (rng_next() % wave_div[g->lvl]) == 0) {
            // all-lane boulder wall - can only jump out, no lane switch escape
            for (int lane = 0; lane < 3; lane++) {
                for (int i = 0; i < MAX_OBSTACLES; i++) {
                    if (!g->obstacles[i].active) {
                        Obstacle_Init(&g->obstacles[i], OBS_BOULDER, lane, SPAWN_Y);
                        break;
                    }
                }
            }
            g->obs_spawn_timer = spawn_gap(g, true);

        } else if (occ < 2) {
            int start = (int)(rng_next() % 3);
            for (int di = 0; di < 3; di++) {
                int lane = (start + di) % 3;
                if (!lane_clear(g, lane)) continue;
                for (int i = 0; i < MAX_OBSTACLES; i++) {
                    if (!g->obstacles[i].active) {
                        ObstacleType_t type = (ObstacleType_t)(rng_next() % 3);
                        Obstacle_Init(&g->obstacles[i], type, lane, SPAWN_Y);
                        break;
                    }
                }
                break;
            }

            if (occ == 0) {
                // first of pair - fire second quickly so they appear together
                int gap = 15 - g->lvl;
                if (gap < 5) gap = 5;
                g->obs_spawn_timer = gap + (int)(rng_next() % 8);
            } else {
                g->obs_spawn_timer = spawn_gap(g, false);
            }

        } else {
            g->obs_spawn_timer = 6;  // all lanes blocked, check again soon
        }
    }

    // coin always in a clear lane so it never sits on top of an obstacle
    g->coin_spawn_timer--;
    if (g->coin_spawn_timer <= 0) {
        int start = (int)(rng_next() % 3);
        for (int di = 0; di < 3; di++) {
            int lane = (start + di) % 3;
            if (!lane_clear(g, lane)) continue;
            for (int i = 0; i < MAX_COINS; i++) {
                if (g->coinPool[i].collected) {
                    Coin_Init(&g->coinPool[i], lane, SPAWN_Y);
                    break;
                }
            }
            break;
        }
        g->coin_spawn_timer = 70 + (int)(rng_next() % 60);
    }
}
