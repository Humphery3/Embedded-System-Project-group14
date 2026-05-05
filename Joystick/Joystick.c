#include "Joystick.h"
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG 57.2957795131f
#define JOYSTICK_ADC_RANGE 4095.0f

void Joystick_Init(Joystick_cfg_t* cfg)
{
    if (!cfg->setup_done) {
        HAL_ADCEx_Calibration_Start(cfg->adc, ADC_SINGLE_ENDED);

        cfg->adc_config.Rank = ADC_REGULAR_RANK_1;
        cfg->adc_config.SamplingTime = cfg->sampling_time;
        cfg->adc_config.SingleDiff = ADC_SINGLE_ENDED;
        cfg->adc_config.OffsetNumber = ADC_OFFSET_NONE;
        cfg->adc_config.Offset = 0;

        cfg->adc_config.Channel = cfg->x_channel;
        HAL_ADC_ConfigChannel(cfg->adc, &cfg->adc_config);

        cfg->adc_config.Channel = cfg->y_channel;
        HAL_ADC_ConfigChannel(cfg->adc, &cfg->adc_config);

        cfg->setup_done = 1;
    }
}

void Joystick_Calibrate(Joystick_cfg_t* cfg)
{
    // Average 50 readings with stick at rest to find the mechanical centre
    uint32_t x_sum = 0, y_sum = 0;
    const int calibration_samples = 50;

    for (int i = 0; i < calibration_samples; i++) {
        cfg->adc_config.Channel = cfg->x_channel;
        HAL_ADC_ConfigChannel(cfg->adc, &cfg->adc_config);
        HAL_ADC_Start(cfg->adc);
        HAL_ADC_PollForConversion(cfg->adc, HAL_MAX_DELAY);
        x_sum += HAL_ADC_GetValue(cfg->adc);
        HAL_ADC_Stop(cfg->adc);

        cfg->adc_config.Channel = cfg->y_channel;
        HAL_ADC_ConfigChannel(cfg->adc, &cfg->adc_config);
        HAL_ADC_Start(cfg->adc);
        HAL_ADC_PollForConversion(cfg->adc, HAL_MAX_DELAY);
        y_sum += HAL_ADC_GetValue(cfg->adc);
        HAL_ADC_Stop(cfg->adc);

        HAL_Delay(10);
    }

    cfg->center_x = x_sum / calibration_samples;
    cfg->center_y = y_sum / calibration_samples;
}

void Joystick_Read(Joystick_cfg_t* cfg, Joystick_t* data)
{
    cfg->adc_config.Channel = cfg->x_channel;
    HAL_ADC_ConfigChannel(cfg->adc, &cfg->adc_config);
    HAL_ADC_Start(cfg->adc);
    HAL_ADC_PollForConversion(cfg->adc, HAL_MAX_DELAY);
    data->x_raw = HAL_ADC_GetValue(cfg->adc);
    HAL_ADC_Stop(cfg->adc);

    cfg->adc_config.Channel = cfg->y_channel;
    HAL_ADC_ConfigChannel(cfg->adc, &cfg->adc_config);
    HAL_ADC_Start(cfg->adc);
    HAL_ADC_PollForConversion(cfg->adc, HAL_MAX_DELAY);
    data->y_raw = HAL_ADC_GetValue(cfg->adc);
    HAL_ADC_Stop(cfg->adc);

    data->x_processed = data->x_raw - cfg->center_x;
    data->y_processed = data->y_raw - cfg->center_y;

    if (abs(data->x_processed) < cfg->deadzone) data->x_processed = 0;
    if (abs(data->y_processed) < cfg->deadzone) data->y_processed = 0;

    data->coord        = Joystick_GetCoord(data->x_processed, data->y_processed, cfg->center_x, cfg->center_y);
    data->coord_mapped = Joystick_MapToCircle(data->coord);

    Polar p = Joystick_GetPolar(data);
    data->angle     = p.angle;
    data->magnitude = p.mag;
    data->direction = Joystick_GetDirection(data->angle, data->magnitude);
}

UserInput Joystick_GetInput(Joystick_t* data)
{
    UserInput input;
    input.direction = data->direction;
    input.magnitude = data->magnitude;
    input.angle = data->angle;
    return input;
}

Direction Joystick_GetDirection(float angle, float magnitude)
{
    if (angle < 0.0f || magnitude < 0.05f) return CENTRE;

    // 0° = North, 90° = East (compass convention)
    if (angle >= 337.5f || angle < 22.5f) return N;
    else if (angle >= 22.5f && angle < 67.5f) return NE;
    else if (angle >= 67.5f && angle < 112.5f) return E;
    else if (angle >= 112.5f && angle < 157.5f) return SE;
    else if (angle >= 157.5f && angle < 202.5f) return S;
    else if (angle >= 202.5f && angle < 247.5f) return SW;
    else if (angle >= 247.5f && angle < 292.5f) return W;
    else return NW;
}

// Normalise centered ADC values to [-1.0, 1.0]. Y is negated so +y = up.
Vector2D Joystick_GetCoord(int16_t x, int16_t y, uint16_t center_x, uint16_t center_y)
{
    float norm_x = (float)x / (float)center_x;
    float norm_y = (float)y / (float)center_y;

    if (norm_x >  1.0f) norm_x =  1.0f;
    if (norm_x < -1.0f) norm_x = -1.0f;
    if (norm_y >  1.0f) norm_y =  1.0f;
    if (norm_y < -1.0f) norm_y = -1.0f;

    Vector2D coord = {norm_x, -norm_y};
    return coord;
}

// Square-to-circle mapping so diagonal deflection feels the same as cardinal.
// Formula from: http://mathproofs.blogspot.co.uk/2005/07/mapping-square-to-circle.html
Vector2D Joystick_MapToCircle(Vector2D coord)
{
    float x = coord.x * sqrtf(1.0f - (coord.y * coord.y) / 2.0f);
    float y = coord.y * sqrtf(1.0f - (coord.x * coord.x) / 2.0f);
    
    Vector2D mapped = {x, y};
    return mapped;
}

// Returns compass-convention polar coords (0° = North, CW). Angle = -1 if centred.
Polar Joystick_GetPolar(Joystick_t* data)
{
    Polar p;

    // Swap axes so atan2 gives compass heading rather than mathematical angle
    float x = data->coord_mapped.y;
    float y = data->coord_mapped.x;

    float mag   = sqrtf(x * x + y * y);
    float angle = RAD2DEG * atan2f(y, x);

    if (angle < 0.0f) angle += 360.0f;
    if (mag < 0.01f)  angle = -1.0f;

    p.mag   = mag;
    p.angle = angle;
    return p;
}

