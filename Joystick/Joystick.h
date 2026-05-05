#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdint.h>
#include <stdlib.h>
#include "main.h"

typedef enum {
    CENTRE = 0,
    N, NE, E, SE, S, SW, W, NW
} Direction;

typedef struct {
    Direction direction;
    float magnitude;   // 0.0 to 1.0
    float angle;       // 0-360 deg, -1 if centred
} UserInput;

// x/y normalised to -1.0 .. 1.0  (North = +y, East = +x)
typedef struct {
    float x;
    float y;
} Vector2D;

typedef struct {
    float mag;    // 0.0 to 1.0
    float angle;  // 0-360 compass, -1 if centred
} Polar;

#define JOYSTICK_DEFAULT_CENTER_X  2048
#define JOYSTICK_DEFAULT_CENTER_Y  2048
#define JOYSTICK_DEADZONE          200
#define JOYSTICK_MAX_VALUE         4095

typedef struct {
    ADC_HandleTypeDef*     adc;
    uint32_t               x_channel;
    uint32_t               y_channel;
    uint32_t               sampling_time;
    uint16_t               center_x;
    uint16_t               center_y;
    uint16_t               deadzone;
    uint8_t                setup_done;
    ADC_ChannelConfTypeDef adc_config;
} Joystick_cfg_t;

typedef struct {
    uint16_t  x_raw;
    uint16_t  y_raw;
    int16_t   x_processed;
    int16_t   y_processed;
    Vector2D  coord;
    Vector2D  coord_mapped;
    float     angle;
    Direction direction;
    float     magnitude;
} Joystick_t;

void Joystick_Init(Joystick_cfg_t* cfg);
void Joystick_Calibrate(Joystick_cfg_t* cfg);  // ~500 ms blocking, hold stick centred
void Joystick_Read(Joystick_cfg_t* cfg, Joystick_t* data);

UserInput  Joystick_GetInput(Joystick_t* data);
Polar      Joystick_GetPolar(Joystick_t* data);
Vector2D   Joystick_GetCoord(int16_t x, int16_t y, uint16_t center_x, uint16_t center_y);
Vector2D   Joystick_MapToCircle(Vector2D coord);
Direction  Joystick_GetDirection(float angle, float magnitude);

#endif /* JOYSTICK_H */
