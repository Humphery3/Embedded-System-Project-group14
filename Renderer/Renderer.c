#include "Renderer.h"
#include "LCD.h"
#include <stdio.h>

#define COL_SKY_DAY   4
#define COL_SKY_NIGHT 9
#define COL_DUNE     12
#define COL_SAND     10
#define COL_HUD       1
#define COL_RED       2
#define COL_YELLOW    6
#define COL_ORANGE    5
#define COL_GOLD     10
#define COL_NAVY      9
#define COL_BLACK     0

#define HORIZON_Y   40
#define VANISH_X    120
#define ROAD_DEPTH  (PLAYER_Y - HORIZON_Y)

static inline int lane_cx(int lane) { return 40 + lane * 80; }

// perspective scale: 0.0 at horizon, 1.0 at player feet
static inline float persp_t(int y) {
    return (float)(y - HORIZON_Y) / (float)ROAD_DEPTH;
}

static ST7789V2_cfg_t *s_cfg;

void Renderer_Init(ST7789V2_cfg_t *cfg) {
    s_cfg = cfg;
}

void Renderer_DrawBackground(bool day) {
    uint8_t sky = day ? COL_SKY_DAY : COL_SKY_NIGHT;

    LCD_Draw_Rect(0, 0, 240, HORIZON_Y, sky, 1);

    // dune circles drawn before the sand rect — sand clips their lower half
    LCD_Draw_Circle( 40, HORIZON_Y, 30, COL_DUNE, 1);
    LCD_Draw_Circle(120, HORIZON_Y, 30, COL_DUNE, 1);
    LCD_Draw_Circle(200, HORIZON_Y, 30, COL_DUNE, 1);

    LCD_Draw_Rect(0, HORIZON_Y, 240, 240 - HORIZON_Y, COL_SAND, 1);

    // non-converging dividers so lanes stay equal width top-to-bottom
    LCD_Draw_Line(80,  HORIZON_Y, 80,  239, COL_DUNE);
    LCD_Draw_Line(160, HORIZON_Y, 160, 239, COL_DUNE);

    LCD_Draw_Rect(0, HORIZON_Y, 240, 2, COL_DUNE, 1);
}

void Renderer_DrawPlayer(Player_t *p) {
    int x = PLAYER_LANE_X[p->lane];

    int jumpOffset = p->jy;

    int rows  = PLAYER_ROWS;
    int yFoot = PLAYER_Y - jumpOffset;
    if (p->sliding) {
        rows  = PLAYER_ROWS / 2;  // crop top half of sprite, looks like a crouch
        yFoot = PLAYER_Y;
    }
    int y = yFoot - rows * PLAYER_SCALE;
    LCD_Draw_Sprite_Scaled(x, y, rows, PLAYER_COLS,
                           (const uint8_t *)player_sprite, PLAYER_SCALE);
}

void Renderer_DrawObstacles(Obstacle_t *obstacles, int count) {
    for (int i = 0; i < count; i++) {
        Obstacle_t *o = &obstacles[i];
        if (!o->active) continue;
        if (o->y < HORIZON_Y || o->y > 250) continue;

        int cx = lane_cx(o->lane);

        switch (o->type) {

            case OBS_BOULDER: {
                int w = OBS_W_MAX;
                int h = OBS_H_ROCK;
                int x = cx - w / 2;
                int y = o->y - h;
                if (x < 0) x = 0;
                if (x + w > 240) w = 240 - x;

                LCD_Draw_Rect(x, y, w, h, COL_DUNE, 1);
                LCD_Draw_Rect(x + 1, y, w / 3, 2, COL_HUD, 1);   // highlight streak on top
                LCD_Draw_Rect(x, y + h - 2, w, 2, COL_BLACK, 1); // shadow at base
                break;
            }

            case OBS_BEAM: {
                int w  = OBS_W_MAX + 4;
                int bh = OBS_H_BEAM;
                int x  = cx - w / 2;
                if (x < 0) x = 0;
                if (x + w > 240) w = 240 - x;

                int beam_top = o->y - 28;
                int post_h   = 18;
                LCD_Draw_Rect(x,         beam_top, 2, post_h, COL_RED, 1);
                LCD_Draw_Rect(x + w - 2, beam_top, 2, post_h, COL_RED, 1);
                LCD_Draw_Rect(x, beam_top, w, bh, COL_RED, 1);
                if (w > 6)
                    LCD_Draw_Rect(x + 2, beam_top + 1, w - 4, 1, COL_YELLOW, 1);
                break;
            }

            case OBS_WALL: {
                int w  = OBS_W_MAX;
                int wh = OBS_H_WALL;
                int x  = cx - w / 2;
                int y  = o->y - wh;
                if (x < 0) x = 0;
                if (x + w > 240) w = 240 - x;
                if (y < HORIZON_Y) y = HORIZON_Y;

                LCD_Draw_Rect(x, y, w, wh, COL_ORANGE, 1);

                if (w > 4) {
                    int third = wh / 3;
                    LCD_Draw_Rect(x, y + third,     w, 1, COL_RED, 1);
                    LCD_Draw_Rect(x, y + 2 * third, w, 1, COL_RED, 1);
                }
                LCD_Draw_Rect(x, y, w, 2, COL_BLACK, 1);
                break;
            }
        }
    }
}

void Renderer_DrawCoins(Coin_t *coins, int count) {
    for (int i = 0; i < count; i++) {
        Coin_t *c = &coins[i];
        if (c->collected) continue;

        float t = persp_t(c->y);
        if (t < 0.0f || t > 1.1f) continue;

        int cx = lane_cx(c->lane);
        int cy = c->y - (int)(t * 16);

        // floor at r=3 so coins stay visible even near the horizon
        int r = 3 + (int)(t * (COIN_RADIUS - 1));

        LCD_Draw_Circle(cx, cy, r + 1, COL_YELLOW, 1);
        LCD_Draw_Circle(cx, cy, r,     COL_GOLD,   1);
        LCD_Draw_Rect(cx - 1, cy - 1, 2, 2, COL_HUD, 1);
    }
}

void Renderer_DrawHUD(int score, int coins) {
    char buf[24];
    LCD_Draw_Rect(0, 214, 240, 26, COL_BLACK, 1);

    snprintf(buf, sizeof(buf), "S:%d", score);
    LCD_printString(buf, 4, 220, COL_HUD, 1);

    snprintf(buf, sizeof(buf), "C:%d", coins);
    LCD_printString(buf, 175, 220, COL_GOLD, 1);
}

void Renderer_DrawMenu(void) {
    LCD_Draw_Rect(0, 0, 240, 240, COL_NAVY, 1);

    LCD_Draw_Circle( 40, 165, 55, COL_DUNE, 1);
    LCD_Draw_Circle(150, 160, 65, COL_DUNE, 1);
    LCD_Draw_Circle(220, 165, 42, COL_DUNE, 1);
    LCD_Draw_Rect(0, 165, 240, 75, COL_SAND, 1);

    LCD_Draw_Line(80,  165, 80,  239, COL_DUNE);
    LCD_Draw_Line(160, 165, 160, 239, COL_DUNE);

    LCD_Draw_Rect( 18, 18, 2, 2, COL_HUD, 1);
    LCD_Draw_Rect( 75, 30, 2, 2, COL_HUD, 1);
    LCD_Draw_Rect(135, 12, 2, 2, COL_HUD, 1);
    LCD_Draw_Rect(185, 38, 2, 2, COL_HUD, 1);
    LCD_Draw_Rect( 52, 52, 2, 2, COL_HUD, 1);
    LCD_Draw_Rect(205, 22, 2, 2, COL_HUD, 1);
    LCD_Draw_Rect(165, 55, 2, 2, COL_HUD, 1);

    LCD_printString("DESERT", 48, 58, COL_YELLOW, 2);
    LCD_printString("DASH", 76, 80, COL_ORANGE, 2);

    LCD_printString("UP=Jump  DOWN=Slide", 8, 112, COL_HUD, 1);
    LCD_printString("LEFT/RIGHT=Lane", 28, 124, COL_HUD, 1);

    LCD_printString("Press BTN3 to Start", 8, 148, COL_YELLOW, 1);
}

void Renderer_DrawGameOver(int score, int coins) {
    char buf[32];

    LCD_Draw_Rect(20, 68, 200, 136, COL_BLACK, 1);

    LCD_Draw_Rect(20,  68, 200,   3, COL_RED, 1);
    LCD_Draw_Rect(20, 201, 200,   3, COL_RED, 1);
    LCD_Draw_Rect(20,  68,   3, 136, COL_RED, 1);
    LCD_Draw_Rect(217, 68,   3, 136, COL_RED, 1);

    LCD_printString("GAME OVER", 50, 86, COL_RED, 2);

    snprintf(buf, sizeof(buf), "Score: %d", score);
    LCD_printString(buf, 55, 130, COL_HUD, 1);

    snprintf(buf, sizeof(buf), "Coins:  %d", coins);
    LCD_printString(buf, 55, 148, COL_GOLD, 1);

    LCD_printString("BTN3 to retry", 42, 178, COL_ORANGE, 1);
}

void Renderer_Present(void) {
    LCD_Refresh(s_cfg);
}
