#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>
#include <stdbool.h>

#define TRANSP_KEY  0x0000

typedef struct {
    const uint16_t *data;
    uint16_t        width;
    uint16_t        height;
    uint8_t         dataSize;
} tImage;

// number of frames
#define FB_IDLE_FRAMES  1
#define FB_RUN_FRAMES   4
#define FB_JUMP_FRAMES  9
#define FB_PUSH_FRAMES  6

#define WG_IDLE_FRAMES  1
#define WG_RUN_FRAMES   4
#define WG_JUMP_FRAMES  9
#define WG_PUSH_FRAMES  6

// fireboy 
extern const tImage fireboy_south;
extern const tImage frame_000, frame_001, frame_002, frame_003;
extern const tImage fb_jump_0, fb_jump_1, fb_jump_2, fb_jump_3, fb_jump_4;
extern const tImage fb_jump_5, fb_jump_6, fb_jump_7, fb_jump_8;
extern const tImage fb_push_0, fb_push_1, fb_push_02, fb_push_3, fb_push_4, fb_push_5;

// watergirl 
extern const tImage wg_idle_0;
extern const tImage wg_run_0, wg_run_1, wg_run_2, wg_run_3;
extern const tImage wg_jump_0, wg_jump_1, wg_jump_2, wg_jump_3;
extern const tImage wg_jump_4, wg_jump_5, wg_jump_6, wg_jump_7, wg_jump_8;
extern const tImage wg_push_0, wg_push_1, wg_push_2, wg_push_3, wg_push_4, wg_push_5;


extern const tImage *fb_idle_frames[FB_IDLE_FRAMES];
extern const tImage *fb_run_frames [FB_RUN_FRAMES];
extern const tImage *fb_jump_frames[FB_JUMP_FRAMES];
extern const tImage *fb_push_frames[FB_PUSH_FRAMES];

extern const tImage *wg_idle_frames[WG_IDLE_FRAMES];
extern const tImage *wg_run_frames [WG_RUN_FRAMES];
extern const tImage *wg_jump_frames[WG_JUMP_FRAMES];
extern const tImage *wg_push_frames[WG_PUSH_FRAMES];

void Sprite_Blit(const tImage *img, int x, int y, bool flip_x);

#endif 
