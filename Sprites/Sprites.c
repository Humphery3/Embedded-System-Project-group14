#include "Sprites.h"
#include "ST7789V2_Driver.h"

// reminder to self: cfg0 declared in main.c
extern ST7789V2_cfg_t cfg0;

// animation frames
const tImage *fb_idle_frames[FB_IDLE_FRAMES] = {
    &fireboy_south,
};
const tImage *fb_run_frames[FB_RUN_FRAMES] = {
    &frame_000, &frame_001, &frame_002, &frame_003,
};
const tImage *fb_jump_frames[FB_JUMP_FRAMES] = {
    &fb_jump_0, &fb_jump_1, &fb_jump_2, &fb_jump_3, &fb_jump_4,
    &fb_jump_5, &fb_jump_6,
};
const tImage *fb_push_frames[FB_PUSH_FRAMES] = {
    &fb_push_0, &fb_push_1, &fb_push_02, &fb_push_3, &fb_push_4, &fb_push_5,
};

const tImage *wg_idle_frames[WG_IDLE_FRAMES] = {
    &wg_idle_0,
};
const tImage *wg_run_frames[WG_RUN_FRAMES] = {
    &wg_run_0, &wg_run_1, &wg_run_2, &wg_run_3,
};
const tImage *wg_jump_frames[WG_JUMP_FRAMES] = {
    &wg_jump_0, &wg_jump_1, &wg_jump_2, &wg_jump_3,
    &wg_jump_4, &wg_jump_5, &wg_jump_6,
};

const tImage *wg_push_frames[WG_PUSH_FRAMES] = {
    &wg_push_0, &wg_push_1, &wg_push_2, &wg_push_3, &wg_push_4, &wg_push_5,
};

// draw sprite directly to LCD, with optional horizontal flip
void Sprite_Blit(const tImage *img, int x, int y, bool flip_x) {
    if (!img || !img->data) return;

    int w = img->width;
    int h = img->height;

    // row buffer — static so it doesn't sit on the stack
    static uint8_t row_buf[480];

    for (int row = 0; row < h; row++) {
        int py = y + row;
        if (py < 0 || py >= 240) continue;

        const uint16_t *row_src = &img->data[row * w];
        int col = 0;

        while (col < w) {
            // skip transparent or off-screen pixels
            while (col < w) {
                int sc = flip_x ? (w - 1 - col) : col;
                int px = x + col;
                if (px >= 0 && px < 240 && row_src[sc] != TRANSP_KEY) break;
                col++;
            }
            if (col >= w) break;

            // collect a run of opaque on-screen pixels
            int px_start = x + col;
            int n = 0;

            while (col < w) {
                int px = x + col;
                if (px < 0 || px >= 240) break;
                int sc = flip_x ? (w - 1 - col) : col;
                uint16_t pixel = row_src[sc];
                if (pixel == TRANSP_KEY) break;
                // big-endian RGB565 for ST7789V2
                row_buf[n * 2    ] = (uint8_t)(pixel >> 8);
                row_buf[n * 2 + 1] = (uint8_t)(pixel & 0xFF);
                n++;
                col++;
            }

            if (n == 0) { col++; continue; }

            int px_end = px_start + n - 1;
            ST7789V2_Set_Address_Window(&cfg0, px_start, py, px_end, py);
            ST7789V2_Send_Command(&cfg0, ST7789_RAMWR);
            ST7789V2_Send_Data_Block(&cfg0, row_buf, (uint32_t)n * 2);
        }
    }
}
